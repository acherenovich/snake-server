#include "game_server.hpp"

#include <boost/crc.hpp>

namespace Core::App::Game
{
    GameServer::GameServer()
    {

    }

    void GameServer::Initialise(const uint8_t serverID)
    {
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

        RespawnSnake(snake);
    }

    void GameServer::OnSessionDisconnected(const UdpSession::Shared & session)
    {
        const auto snake = sessions_[session];
        snakes_.erase(snake);

        sessions_.erase(session);
    }

    void GameServer::OnMessage(const UdpSession::Shared & session, const std::vector<std::uint8_t>& data)
    {
        using namespace Utils::Legacy::Game::Net;

        MessageHeader header;
        if (!ParseHeader(std::span(data.data(), data.size()), header))
        {
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

            RequestFullUpdatePayload rq;
            if (!ReadRequestFullUpdatePayload(reader, rq))
            {
                // даже если не смогли прочитать — просто обычный full
                rq.flags = 0;
            }

            auto& state = netState_[session];
            state.fullUpdateAllSegmentsNext = (rq.flags & RequestFullUpdateFlag_AllSegments) != 0;

            fullUpdates_.insert(session);
            return;
        }

        if (type == MessageType::ClientInput)
        {
            ByteReader reader(payloadSpan);

            ClientInputPayload input;
            if (!reader.ReadPod(input))
            {
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

        ByteWriter payload(8 * 1024);

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

            // add to pending removes to resend few ticks
            PendingRemove pr{};
            pr.type = state.lastType[entityID];
            pr.retries = 20; // ~0.6 сек при 32 апдейтах/сек, “дожимаем” удаление

            if (!state.pendingRemoves.contains(entityID))
            {
                state.pendingRemoves.emplace(entityID, pr);
            }

            // baseline чистим сразу (чтобы не пытаться “обновлять” удалённое)
            state.lastHash.erase(entityID);
            state.lastType.erase(entityID);

            // и сразу пишем remove в текущий payload тоже
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

            if (!snake->CanSee(s))
                return;

            const auto d = std::hypot(s->GetPosition().x - snake->GetPosition().x,
                                     s->GetPosition().y - snake->GetPosition().y);
            if (d > sendRadius)
                return;

            visibleNow.insert(s->EntityID());

            SnakeState sstate{}; // IMPORTANT: zero-init
            sstate.headX = s->GetPosition().x;
            sstate.headY = s->GetPosition().y;
            sstate.experience = s->GetExperience();
            sstate.totalSegments = static_cast<std::uint16_t>(s->Segments().size());

            const auto samples = SampleSnakeSegments(s);
            sstate.sampleCount = static_cast<std::uint8_t>(samples.size());

            std::uint32_t hash = 0;
            hash ^= HashBytes(&sstate, sizeof(sstate));
            for (const auto& v : samples)
            {
                hash ^= HashBytes(&v.x, sizeof(v.x));
                hash ^= HashBytes(&v.y, sizeof(v.y));
            }

            const bool known = state.lastHash.contains(s->EntityID());
            const std::uint32_t oldHash = known ? state.lastHash[s->EntityID()] : 0;

            if (!known)
            {
                EntityEntryHeader entry{};
                entry.type = EntityType::Snake;
                entry.flags = EntityFlags::New;
                entry.entityID = s->EntityID();
                payload.WritePod(entry);

                payload.WritePod(sstate);
                for (const auto& v : samples)
                    payload.WriteVector2f(v);

                state.lastHash[s->EntityID()] = hash;
                state.lastType[s->EntityID()] = EntityType::Snake;
            }
            else if (hash != oldHash)
            {
                EntityEntryHeader entry{};
                entry.type = EntityType::Snake;
                entry.flags = EntityFlags::Update;
                entry.entityID = s->EntityID();
                payload.WritePod(entry);

                payload.WritePod(sstate);
                for (const auto& v : samples)
                    payload.WriteVector2f(v);

                state.lastHash[s->EntityID()] = hash;
                state.lastType[s->EntityID()] = EntityType::Snake;
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

            FoodState fs{}; // IMPORTANT: zero-init
            fs.x = f->GetPosition().x;
            fs.y = f->GetPosition().y;
            fs.power = f->GetPower();
            fs.color = *reinterpret_cast<const Color*>(&f->GetColor());
            fs.killed = 0;

            const std::uint32_t hash = HashBytes(&fs, sizeof(fs));

            const bool known = state.lastHash.contains(f->EntityID());
            const std::uint32_t oldHash = known ? state.lastHash[f->EntityID()] : 0;

            if (!known)
            {
                EntityEntryHeader entry{};
                entry.type = EntityType::Food;
                entry.flags = EntityFlags::New;
                entry.entityID = f->EntityID();
                payload.WritePod(entry);

                payload.WritePod(fs);

                state.lastHash[f->EntityID()] = hash;
                state.lastType[f->EntityID()] = EntityType::Food;
            }
            else if (hash != oldHash)
            {
                EntityEntryHeader entry{};
                entry.type = EntityType::Food;
                entry.flags = EntityFlags::Update;
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

        ByteWriter payload(16 * 1024);
        WriteFullUpdateHeader(payload, snake->EntityID());

        // snapshot baseline
        state.lastHash.clear();
        state.lastType.clear();
        state.pendingRemoves.clear(); // full snapshot = удалений “доехать” больше не нужно

        auto AddSnake = [&](const EntitySnake::Shared& s)
        {
            if (!s || s->IsKilled())
                return;

            if (!snake->CanSee(s))
                return;

            const auto d = std::hypot(s->GetPosition().x - snake->GetPosition().x,
                                     s->GetPosition().y - snake->GetPosition().y);
            if (d > sendRadius)
                return;

            visibleNow.insert(s->EntityID());

            EntityEntryHeader entry{};
            entry.type = EntityType::Snake;
            entry.flags = EntityFlags::New;
            entry.entityID = s->EntityID();
            payload.WritePod(entry);

            SnakeState sstate{}; // IMPORTANT: zero-init
            sstate.headX = s->GetPosition().x;
            sstate.headY = s->GetPosition().y;
            sstate.experience = s->GetExperience();
            sstate.totalSegments = static_cast<std::uint16_t>(s->Segments().size());

            std::vector<sf::Vector2f> samples;

            const bool isPlayer = (s->EntityID() == snake->EntityID());
            const bool sendAllForPlayer = (isPlayer && state.fullUpdateAllSegmentsNext);

            if (sendAllForPlayer)
            {
                const auto& segs = s->Segments();
                samples.assign(segs.begin(), segs.end());
            }
            else
            {
                samples = SampleSnakeSegments(s);
            }

            // important: sampleCount must never exceed totalSegments
            if (samples.size() > static_cast<std::size_t>(sstate.totalSegments))
            {
                samples.resize(sstate.totalSegments);
            }

            sstate.sampleCount = static_cast<std::uint8_t>(samples.size());

            payload.WritePod(sstate);
            for (const auto& v : samples)
                payload.WriteVector2f(v);

            std::uint32_t hash = 0;
            hash ^= HashBytes(&sstate, sizeof(sstate));
            for (const auto& v : samples)
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

            FoodState fs{}; // IMPORTANT: zero-init
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

        // only applies once
        state.fullUpdateAllSegmentsNext = false;
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
        if(CheckBoundsCollision(head, Utils::Legacy::Game::AreaCenter, Utils::Legacy::Game::AreaRadius - snake->GetRadius(true)))
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

        for(auto& target : snakes_) {
            if(snake == target || target->IsKilled())
                continue;

            if (CheckCollision(snake->GetPosition(), target->GetPosition(), snake->GetRadius(true) + target->GetRadius(true)))
            {
                if(target->GetExperience() >= snake->GetExperience()) {
                    KillSnake(snake);
                    return;
                }
            }

            for(auto & part: target->Segments())
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

            // Преобразуем список сегментов в вектор для удобного доступа по индексу
            std::vector segmentVector(segments.begin(), segments.end());

            const uint32_t numFoods = snake->GetExperience() / 3;
            const auto numSegments = static_cast<int>(segmentVector.size());

            for (int i = 0; i < numFoods; i++)
            {
                // Выбираем случайный индекс сегмента
                const auto randomIndex = GetRandomInt(0, numSegments - 1);

                // Получаем позицию выбранного сегмента
                sf::Vector2f segmentPosition = segmentVector[randomIndex];

                // Генерируем случайную позицию в небольшом радиусе вокруг сегмента
                const float spawnRadius = randomIndex == 0 ? snake->GetRadius(true) : snake->GetRadius(false);
                sf::Vector2f position = GetRandomVector2fInSphere(segmentPosition, spawnRadius);

                // Создаём новый объект еды в сгенерированной позиции
                auto food = std::make_shared<EntityFood>(frame_, position);
                food->SetEntityID(nextEntityID_++);
                foods_.insert(food);
            }
        }

        killedSnakes_.clear();
    }

    void GameServer::GenerateFoods()
    {
        for(auto i = foods_.size(); i < Utils::Legacy::Game::FoodCount; i++)
        {
            const auto position = GetRandomVector2fInSphere(Utils::Legacy::Game::AreaCenter, Utils::Legacy::Game::AreaRadius - 10.f);
            auto food = std::make_shared<EntityFood>(frame_, position);
            food->SetEntityID(nextEntityID_++);
            foods_.insert(food);
        }
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

    std::uint32_t HashFloat(const float v)
    {
        return HashBytes(&v, sizeof(v));
    }

    std::uint32_t HashVector2f(const sf::Vector2f& v)
    {
        std::uint32_t h = 0;
        h ^= HashFloat(v.x);
        h ^= (HashFloat(v.y) << 1);
        return h;
    }

    std::vector<sf::Vector2f> SampleSnakeSegments(const Utils::Legacy::Game::Entity::Snake::Shared& snake)
    {
        const auto& segments = snake->Segments();
        const std::size_t n = segments.size();

        std::vector<sf::Vector2f> out;

        if (n == 0)
        {
            return out;
        }

        // we can never send more samples than we have segments
        const std::size_t maxSamples = std::min<std::size_t>(12, n);
        out.reserve(maxSamples);

        // copy to vector for index access
        const std::vector<sf::Vector2f> segVec(segments.begin(), segments.end());

        // first 6 (or less if n < 6)
        const std::size_t firstCount = std::min<std::size_t>(6, maxSamples);
        for (std::size_t i = 0; i < firstCount; ++i)
        {
            out.push_back(segVec[i]);
        }

        if (maxSamples <= firstCount)
        {
            return out;
        }

        // remaining points evenly distributed over [firstCount .. n-1]
        const std::size_t remaining = maxSamples - firstCount;

        const std::size_t start = firstCount;
        const std::size_t end   = n - 1;

        if (start >= n)
        {
            return out;
        }

        const std::size_t range = end - start;

        if (remaining == 1)
        {
            out.push_back(segVec[end]);
            return out;
        }

        for (std::size_t k = 0; k < remaining; ++k)
        {
            const float t = static_cast<float>(k) / static_cast<float>(remaining - 1);
            const std::size_t idx = start + static_cast<std::size_t>(std::round(t * static_cast<float>(range)));

            if (idx < n)
            {
                out.push_back(segVec[idx]);
            }
        }

        return out;
    }
}