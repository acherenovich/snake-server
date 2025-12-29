#include "game_server.hpp"

#include <boost/crc.hpp>
#include <cmath>
#include <ranges>
#include <vector>

#include "logging.hpp"

namespace Core::App::Game
{
    GameServer::GameServer()
    {
    }

    void GameServer::Initialise(const uint8_t serverID)
    {
        serverID_ = serverID;

        Utils::Net::Udp::ServerConfig cfg;
        cfg.address = "0.0.0.0";
        cfg.port = 7777 + serverID;
        cfg.mode = Utils::Net::Udp::Mode::Bytes;
        cfg.ioThreads = 2;

        udpServer_ = UdpServer::Create(cfg, shared_from_this());
    }

    void GameServer::ProcessTick()
    {
        Logic::ProcessTick();

        // send updates at 32 tickrate (logic is 64)
        if (const bool shouldSendUpdate = (frame_ % 2 == 0); !shouldSendUpdate)
            return;

        for (auto& snake : snakes_)
        {
            ProcessSnake(snake);
        }

        ProcessKills();
        GenerateFoods();

        for (auto& [session, snake] : sessions_)
        {
            // first: if this session requested snapshot(s), send them out-of-band
            auto& state = netState_[session];
            if (!state.pendingSnakeSnapshots.empty())
            {
                // send a few per tick to avoid worst-case spikes
                std::uint32_t sent = 0;
                constexpr std::uint32_t perTickLimit = 8;
                for (auto it = state.pendingSnakeSnapshots.begin(); it != state.pendingSnakeSnapshots.end() && sent < perTickLimit; )
                {
                    const auto entityID = *it;
                    it = state.pendingSnakeSnapshots.erase(it);
                    SendSnakeSnapshot(session, entityID);
                    sent++;
                }
            }

            if (fullUpdates_.contains(session))
                SendFullUpdate(session, snake);
            else
                SendPartialUpdate(session, snake);
        }
        fullUpdates_.clear();

        udpServer_->ProcessTick();
    }

    void GameServer::OnSessionConnected(const UdpSession::Shared & session)
    {
        const auto snake = std::make_shared<EntitySnake>();
        snake->SetEntityID(nextEntityID_++);
        snakes_.insert(snake);
        sessions_[session] = snake;
        sessionsByID_[session->SessionId()] = session;

        RespawnSnake(snake);
    }

    void GameServer::OnSessionDisconnected(const UdpSession::Shared & session)
    {
        const auto snake = sessions_[session];
        snakes_.erase(snake);

        const auto sessionID = session->SessionId();
        sessionsByID_.erase(sessionID);
        players_.erase(sessionID);
        sessions_.erase(session);
        netState_.erase(session);
    }

    void GameServer::OnMessage(const UdpSession::Shared & session, const std::vector<std::uint8_t>& data)
    {
        using namespace Utils::Legacy::Game::Net;

        MessageHeader header{};
        const auto parseErr = ParseHeaderDetailed(std::span(data.data(), data.size()), header);
        if (parseErr != ParseError::Ok)
        {
            Log()->Warning("[Net] Dropped client packet: ParseHeaderDetailed failed. err={} bytes={}",
                           ParseErrorToString(parseErr), data.size());
            return;
        }

        const auto type = static_cast<MessageType>(header.type);

        const auto payloadSpan = std::span(
            data.data() + sizeof(MessageHeader),
            header.payloadBytes
        );

        if (type == MessageType::RequestFullUpdate)
        {
            ByteReader reader(payloadSpan);

            RequestFullUpdatePayload rq{};
            if (!ReadRequestFullUpdatePayload(reader, rq))
            {
                rq.flags = 0;
            }

            auto& state = netState_[session];
            state.fullUpdateAllSegmentsNext = (rq.flags & RequestFullUpdateFlag_AllSegments) != 0;

            fullUpdates_.insert(session);
            return;
        }

        if (type == MessageType::RequestSnakeSnapshot)
        {
            ByteReader reader(payloadSpan);

            RequestSnakeSnapshotPayload rq{};
            if (!ReadRequestSnakeSnapshotPayload(reader, rq))
            {
                Log()->Warning("[Net] Dropped RequestSnakeSnapshot: read failed");
                return;
            }

            auto& state = netState_[session];
            state.pendingSnakeSnapshots.insert(rq.entityID);
            return;
        }

        if (type == MessageType::ClientInput)
        {
            ByteReader reader(payloadSpan);

            ClientInputPayload input{};
            if (!reader.ReadPod(input))
            {
                Log()->Warning("[Net] Dropped ClientInput: read failed");
                return;
            }

            const auto snake = sessions_[session];
            snake->SetDestination({ input.destinationX, input.destinationY });
            return;
        }
    }

    void GameServer::SendPartialUpdate(const UdpSession::Shared & session, const EntitySnake::Shared & snake)
    {
        using namespace Utils::Legacy::Game::Net;

        auto& state = netState_[session];
        state.updateSeq++;

        const float visibleRadius = EntitySnake::camera_radius * snake->GetZoom();
        const float sendRadius = visibleRadius * (1.0f + visibilityPaddingPercent_);

        std::unordered_set<std::uint32_t> visibleNow;
        visibleNow.reserve(snakes_.size() + foods_.size());

        ByteWriter payload(32 * 1024);

        // --------- 1) SEND PENDING REMOVES FIRST (retries) ----------
        for (auto it = state.pendingRemoves.begin(); it != state.pendingRemoves.end(); )
        {
            EntityEntryHeader entry{};
            entry.type = it->second.type;
            entry.flags = EntityFlags::Remove;
            entry.entityID = it->first;

            payload.WritePod(entry);

            if (it->second.retries > 0)
            {
                it->second.retries--;
            }

            if (it->second.retries == 0)
            {
                it = state.pendingRemoves.erase(it);
            }
            else
            {
                ++it;
            }
        }

        auto QueueRemove = [&](const std::uint32_t entityID)
        {
            if (!state.lastType.contains(entityID))
            {
                return;
            }

            PendingRemove pr{};
            pr.type = state.lastType[entityID];
            pr.retries = 20;

            if (!state.pendingRemoves.contains(entityID))
            {
                state.pendingRemoves.emplace(entityID, pr);
            }

            state.lastHash.erase(entityID);
            state.lastType.erase(entityID);

            EntityEntryHeader entry{};
            entry.type = pr.type;
            entry.flags = EntityFlags::Remove;
            entry.entityID = entityID;

            payload.WritePod(entry);
        };

        auto ProcessSnakeVisible = [&](const EntitySnake::Shared& s)
        {
            if (!s || s->IsKilled())
                return;

            if (!IsSnakeVisibleByAnySegment(snake, s, sendRadius))
                return;

            visibleNow.insert(s->EntityID());

            const bool known = state.lastHash.contains(s->EntityID());

            EntityEntryHeader entry{};
            entry.type = EntityType::Snake;
            entry.entityID = s->EntityID();

            SnakeState sstate{};
            sstate.headX = s->GetPosition().x;
            sstate.headY = s->GetPosition().y;
            sstate.experience = s->GetExperience();
            sstate.totalSegments = static_cast<std::uint16_t>(s->Segments().size());

            std::vector<sf::Vector2f> points;

            if (!known)
            {
                // NEW snake -> send FULL segments in the same packet (may exceed MTU, ok with UDP reassembly)
                entry.flags = EntityFlags::New;
                points = GetSnakeFullSegments(s);
                sstate.pointsKind = SnakePointsKind::FullSegments;
                sstate.pointsCount = static_cast<std::uint16_t>(points.size());
            }
            else
            {
                // UPDATE -> validation samples only (radius-based)
                entry.flags = EntityFlags::Update;
                points = SampleSnakeValidationPoints(s);
                sstate.pointsKind = SnakePointsKind::ValidationSamples;
                sstate.pointsCount = static_cast<std::uint16_t>(points.size());
            }

            payload.WritePod(entry);
            payload.WritePod(sstate);
            for (const auto& v : points)
                payload.WriteVector2f(v);

            // hash for delta filtering
            std::uint32_t hash = 0;
            hash ^= HashBytes(&sstate, sizeof(sstate));
            for (const auto& v : points)
            {
                hash ^= HashBytes(&v.x, sizeof(v.x));
                hash ^= HashBytes(&v.y, sizeof(v.y));
            }

            if (!known)
            {
                state.lastHash[s->EntityID()] = hash;
                state.lastType[s->EntityID()] = EntityType::Snake;
            }
            else
            {
                const std::uint32_t oldHash = state.lastHash[s->EntityID()];
                if (hash != oldHash)
                {
                    state.lastHash[s->EntityID()] = hash;
                    state.lastType[s->EntityID()] = EntityType::Snake;
                }
                else
                {
                    // no change -> rollback write? too complex; keep sending updates as head likely changes anyway.
                    // we keep hash updated.
                }
            }
        };

        auto ProcessFoodVisible = [&](const EntityFood::Shared& f)
        {
            if (!f || f->IsKilled())
                return;

            if (!snake->CanSee(f))
                return;

            const auto d = std::hypot(f->GetPosition().x - snake->GetPosition().x,
                                     f->GetPosition().y - snake->GetPosition().y);
            if (d > sendRadius)
                return;

            visibleNow.insert(f->EntityID());

            FoodState fs{};
            fs.x = f->GetPosition().x;
            fs.y = f->GetPosition().y;
            fs.power = f->GetPower();
            fs.color = *reinterpret_cast<const Color*>(&f->GetColor());
            fs.killed = 0;

            const std::uint32_t hash = HashBytes(&fs, sizeof(fs));

            const bool known = state.lastHash.contains(f->EntityID());
            const std::uint32_t oldHash = known ? state.lastHash[f->EntityID()] : 0;

            if (!known || hash != oldHash)
            {
                EntityEntryHeader entry{};
                entry.type = EntityType::Food;
                entry.flags = known ? EntityFlags::Update : EntityFlags::New;
                entry.entityID = f->EntityID();
                payload.WritePod(entry);

                payload.WritePod(fs);

                state.lastHash[f->EntityID()] = hash;
                state.lastType[f->EntityID()] = EntityType::Food;
            }
        };

        for (const auto& s : snakes_)
            ProcessSnakeVisible(s);

        for (const auto& f : foods_)
            ProcessFoodVisible(f);

        // --------- 2) REMOVES: anything that was visible, but now not visible ---------
        for (const auto oldID : state.lastVisible)
        {
            if (!visibleNow.contains(oldID))
            {
                QueueRemove(oldID);
            }
        }

        state.lastVisible = std::move(visibleNow);

        if (payload.Data().empty())
        {
            return;
        }

        const auto msg = BuildMessage(MessageType::PartialUpdate,
                                      state.updateSeq,
                                      frame_,
                                      payload.Data());

        session->Send(msg);
    }

    void GameServer::SendFullUpdate(const UdpSession::Shared & session, const EntitySnake::Shared & snake)
    {
        using namespace Utils::Legacy::Game::Net;

        auto& state = netState_[session];
        state.updateSeq++;

        const float visibleRadius = EntitySnake::camera_radius * snake->GetZoom();
        const float sendRadius = visibleRadius * (1.0f + visibilityPaddingPercent_);

        std::unordered_set<std::uint32_t> visibleNow;
        visibleNow.reserve(snakes_.size() + foods_.size());

        ByteWriter payload(128 * 1024);
        WriteFullUpdateHeader(payload, snake->EntityID());

        // snapshot baseline
        state.lastHash.clear();
        state.lastType.clear();
        state.pendingRemoves.clear();

        auto AddSnake = [&](const EntitySnake::Shared& s)
        {
            if (!s || s->IsKilled())
                return;

            if (!IsSnakeVisibleByAnySegment(snake, s, sendRadius))
                return;

            visibleNow.insert(s->EntityID());

            EntityEntryHeader entry{};
            entry.type = EntityType::Snake;
            entry.flags = EntityFlags::New;
            entry.entityID = s->EntityID();
            payload.WritePod(entry);

            SnakeState sstate{};
            sstate.headX = s->GetPosition().x;
            sstate.headY = s->GetPosition().y;
            sstate.experience = s->GetExperience();
            sstate.totalSegments = static_cast<std::uint16_t>(s->Segments().size());

            // FullUpdate: ALWAYS send full segments for every visible snake (fixes "6 parts" issue)
            const auto points = GetSnakeFullSegments(s);
            sstate.pointsKind = SnakePointsKind::FullSegments;
            sstate.pointsCount = static_cast<std::uint16_t>(points.size());

            payload.WritePod(sstate);
            for (const auto& v : points)
                payload.WriteVector2f(v);

            std::uint32_t hash = 0;
            hash ^= HashBytes(&sstate, sizeof(sstate));
            for (const auto& v : points)
            {
                hash ^= HashBytes(&v.x, sizeof(v.x));
                hash ^= HashBytes(&v.y, sizeof(v.y));
            }

            state.lastHash[s->EntityID()] = hash;
            state.lastType[s->EntityID()] = EntityType::Snake;
        };

        auto AddFood = [&](const EntityFood::Shared& f)
        {
            if (!f || f->IsKilled())
                return;

            if (!snake->CanSee(f))
                return;

            const auto d = std::hypot(f->GetPosition().x - snake->GetPosition().x,
                                     f->GetPosition().y - snake->GetPosition().y);
            if (d > sendRadius)
                return;

            visibleNow.insert(f->EntityID());

            EntityEntryHeader entry{};
            entry.type = EntityType::Food;
            entry.flags = EntityFlags::New;
            entry.entityID = f->EntityID();
            payload.WritePod(entry);

            FoodState fs{};
            fs.x = f->GetPosition().x;
            fs.y = f->GetPosition().y;
            fs.power = f->GetPower();
            fs.color = *reinterpret_cast<const Color*>(&f->GetColor());
            fs.killed = 0;

            payload.WritePod(fs);

            const std::uint32_t hash = HashBytes(&fs, sizeof(fs));
            state.lastHash[f->EntityID()] = hash;
            state.lastType[f->EntityID()] = EntityType::Food;
        };

        for (const auto& s : snakes_)
            AddSnake(s);

        for (const auto& f : foods_)
            AddFood(f);

        state.lastVisible = std::move(visibleNow);

        const auto msg = BuildMessage(MessageType::FullUpdate,
                                      state.updateSeq,
                                      frame_,
                                      payload.Data());

        session->Send(msg);

        state.fullUpdateAllSegmentsNext = false;
    }

    void GameServer::SendSnakeSnapshot(const UdpSession::Shared& session, const std::uint32_t entityID)
    {
        using namespace Utils::Legacy::Game::Net;

        // find snake by entityID (visible or not - snapshot used for repair, but we still send full data if exists)
        EntitySnake::Shared targetSnake;
        for (const auto& s : snakes_)
        {
            if (s && s->EntityID() == entityID && !s->IsKilled())
            {
                targetSnake = s;
                break;
            }
        }

        if (!targetSnake)
        {
            // if snake doesn't exist (already removed), send nothing.
            return;
        }

        ByteWriter payload(64 * 1024);

        EntityEntryHeader entry{};
        entry.type = EntityType::Snake;
        entry.flags = EntityFlags::New;
        entry.entityID = entityID;
        payload.WritePod(entry);

        SnakeState sstate{};
        sstate.headX = targetSnake->GetPosition().x;
        sstate.headY = targetSnake->GetPosition().y;
        sstate.experience = targetSnake->GetExperience();
        sstate.totalSegments = static_cast<std::uint16_t>(targetSnake->Segments().size());

        const auto points = GetSnakeFullSegments(targetSnake);
        sstate.pointsKind = SnakePointsKind::FullSegments;
        sstate.pointsCount = static_cast<std::uint16_t>(points.size());

        payload.WritePod(sstate);
        for (const auto& v : points)
            payload.WriteVector2f(v);

        auto& state = netState_[session];
        state.updateSeq++;

        const auto msg = BuildMessage(MessageType::SnakeSnapshot,
                                      state.updateSeq,
                                      frame_,
                                      payload.Data());

        session->Send(msg);
    }

    void GameServer::ProcessSnake(const EntitySnake::Shared & snake)
    {
        if (snake->IsKilled())
        {
            if (snake->CanRespawn(frame_))
                RespawnSnake(snake);
            return;
        }

        const auto head = snake->TryMove();
        if (CheckBoundsCollision(head, Utils::Legacy::Game::AreaCenter, Utils::Legacy::Game::AreaRadius - snake->GetRadius(true)))
        {
            KillSnake(snake);
            return;
        }

        snake->AcceptMove(head);

        std::erase_if(foods_, [&](const EntityFood::Shared& food)
        {
            if (CheckCollision(food->GetPosition(),
                               snake->GetPosition(),
                               snake->GetRadius(true) + food->GetRadius()))
            {
                snake->AddExperience(food->GetPower());
                return true;
            }

            return false;
        });

        for (auto& target : snakes_)
        {
            if (snake == target || target->IsKilled())
                continue;

            if (CheckCollision(snake->GetPosition(), target->GetPosition(), snake->GetRadius(true) + target->GetRadius(true)))
            {
                if (target->GetExperience() >= snake->GetExperience())
                {
                    KillSnake(snake);
                    return;
                }
            }

            for (auto & part : target->Segments())
            {
                if (CheckCollision(snake->GetPosition(), part, snake->GetRadius(true) + target->GetRadius(false)))
                {
                    KillSnake(snake);
                    return;
                }
            }
        }

        snake->RecalculateLength();
    }

    void GameServer::RespawnSnake(const EntitySnake::Shared & snake)
    {
        const auto start = GetRandomVector2fInSphere(Utils::Legacy::Game::AreaCenter, Utils::Legacy::Game::AreaRadius - 10.f);

        snake->Respawn(frame_, start);

        for (const auto& [session, sessionSnake] : sessions_)
        {
            if (sessionSnake == snake)
            {
                fullUpdates_.insert(session);
                break;
            }
        }
    }

    void GameServer::KillSnake(const EntitySnake::Shared & snake)
    {
        if (snake->IsKilled())
            return;

        snake->Kill(frame_);
        killedSnakes_.insert(snake);
    }

    void GameServer::ProcessKills()
    {
        for (auto& snake : killedSnakes_)
        {
            auto & segments = snake->Segments();
            std::vector segmentVector(segments.begin(), segments.end());

            const uint32_t numFoods = snake->GetExperience() / 15;
            const auto numSegments = static_cast<int>(segmentVector.size());

            for (int i = 0; i < static_cast<int>(numFoods); i++)
            {
                const auto randomIndex = GetRandomInt(0, numSegments - 1);
                const sf::Vector2f segmentPosition = segmentVector[randomIndex];

                const float spawnRadius = randomIndex == 0 ? snake->GetRadius(true) : snake->GetRadius(false);
                const sf::Vector2f position = GetRandomVector2fInSphere(segmentPosition, spawnRadius);

                auto food = std::make_shared<EntityFood>(frame_, position, 10);
                food->SetEntityID(nextEntityID_++);
                foods_.insert(food);
            }
        }

        killedSnakes_.clear();
    }

    void GameServer::GenerateFoods()
    {
        for (auto i = foods_.size(); i < Utils::Legacy::Game::FoodCount; i++)
        {
            const auto position = GetRandomVector2fInSphere(Utils::Legacy::Game::AreaCenter, Utils::Legacy::Game::AreaRadius - 10.f);
            auto food = std::make_shared<EntityFood>(frame_, position);
            food->SetEntityID(nextEntityID_++);
            foods_.insert(food);
        }
    }

    uint32_t GameServer::GetServerID() const
    {
        return serverID_;
    }

    uint32_t GameServer::GetPlayersCount() const
    {
        return sessions_.size();
    }

    void GameServer::SetSSIDPlayer(const uint64_t ssid, const Player::Shared & player)
    {
        Log()->Debug("SetSSIDPlayer serverId {} session {} connected with {}", serverID_, ssid, player->Model()->GetLogin());

        // for (const auto & [ssid, assignedPlayer] : players_)
        // {
        //     if (assignedPlayer == player)
        //     {
        //         session->Close();
        //     }
        // }

        // for (const auto &session: sessions_ | std::views::keys)
        // {
        //     if (session->SessionId() == ssid)
        //     {
        //         Log()->Debug("Success SetSSIDPlayer");
        //         players_[ssid] = player;
        //     }
        // }

        players_[ssid] = player;
    }

    std::unordered_map<Player::Shared, uint32_t> GameServer::GetLeaderboard()
    {
        std::unordered_map<Player::Shared, uint32_t> leaderboard;

        Log()->Debug("GetLeaderboard: {} {}", serverID_, players_.size());

        for (const auto & [ssid, player] : players_)
        {
            if (!sessionsByID_.contains(ssid))
                continue;

            const auto session = sessionsByID_[ssid];

            const auto snake = sessions_[session];
            if (!snake->IsKilled())
            {
                leaderboard[player] = snake->GetExperience();
            }
        }

        return leaderboard;
    }

    GameServer::Shared GameServer::Create(const BaseServiceContainer * parent, const uint8_t serverID)
    {
        const auto obj = std::make_shared<GameServer>();
        obj->SetupContainer(parent);
        obj->Initialise(serverID);
        return obj;
    }

    std::uint32_t HashBytes(const void* const data, const std::size_t size)
    {
        boost::crc_32_type crc;
        crc.process_bytes(data, size);
        return crc.checksum();
    }

    std::vector<sf::Vector2f> GetSnakeFullSegments(const Utils::Legacy::Game::Entity::Snake::Shared& snake)
    {
        std::vector<sf::Vector2f> out;
        const auto& segments = snake->Segments();
        out.assign(segments.begin(), segments.end());
        return out;
    }

    std::vector<sf::Vector2f> SampleSnakeValidationPoints(const Utils::Legacy::Game::Entity::Snake::Shared& snake)
    {
        std::vector<sf::Vector2f> out;

        const auto& segments = snake->Segments();
        if (segments.empty())
        {
            return out;
        }

        const std::vector<sf::Vector2f> segVec(segments.begin(), segments.end());
        const std::size_t n = segVec.size();

        out.reserve(n);

        const float minDist = snake->GetRadius(false);

        sf::Vector2f last = segVec[0];
        out.push_back(last);

        for (std::size_t i = 1; i < n; ++i)
        {
            const auto& p = segVec[i];
            const float dx = p.x - last.x;
            const float dy = p.y - last.y;
            const float dist = std::hypot(dx, dy);

            if (dist >= minDist)
            {
                out.push_back(p);
                last = p;
            }
        }

        // ensure tail
        if (n >= 2)
        {
            const auto& tail = segVec.back();
            if (out.empty() || (out.back().x != tail.x || out.back().y != tail.y))
            {
                out.push_back(tail);
            }
        }

        return out;
    }

    bool IsSnakeVisibleByAnySegment(const EntitySnake::Shared& viewer, const EntitySnake::Shared& target, const float radius)
    {
        if (!viewer || !target)
            return false;

        const auto viewerPos = viewer->GetPosition(); // viewer head
        const float r2 = radius * radius;

        // Check head first (fast path)
        {
            const auto head = target->GetPosition();
            const float dx = head.x - viewerPos.x;
            const float dy = head.y - viewerPos.y;
            if ((dx * dx + dy * dy) <= r2)
                return true;
        }

        const auto& segments = target->Segments();
        for (const auto& seg : segments)
        {
            const float dx = seg.x - viewerPos.x;
            const float dy = seg.y - viewerPos.y;
            if ((dx * dx + dy * dy) <= r2)
                return true;
        }

        return false;
    }

} // namespace Core::App::Game