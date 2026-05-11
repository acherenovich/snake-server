#include "controller.hpp"

namespace Core::App::Game
{
    void Controller::Initialise() {}

    void Controller::OnAllServicesLoaded()
    {
        IFace().Register<Interface::Controller>(shared_from_this());
    }

    void Controller::OnAllInterfacesLoaded()
    {
        websocket_      = IFace().Get<WsServer>();
        internalServer_ = IFace().Get<IntServer>();
        database_       = IFace().Get<Database>();

        internalServer_->RegisterMessage(
            "internal::register",
            [this](const Client::Shared& client, const Message::Shared& msg) {
                HandleRegister(client, msg);
            });

        internalServer_->RegisterMessage(
            "internal::stats",
            [this](const Client::Shared& client, const Message::Shared& msg) {
                HandleStats(client, msg);
            });

        internalServer_->RegisterClientsCallback(
            [this](const Client::Shared& client, Client::Events event) {
                if (event == Client::Events::ClientDisconnected)
                    HandleReplicaDisconnected(client);
            });
    }

    void Controller::ProcessTick() {}

    std::map<uint32_t, Interface::GameServer::Shared> Controller::GetGameServers() const
    {
        std::map<uint32_t, Interface::GameServer::Shared> result;
        for (const auto& s : gameServers_)
            result[s->GetServerID()] = s;
        return result;
    }

    void Controller::HandleRegister(const Client::Shared& client, const Message::Shared& msg)
    {
        auto& body = msg->GetBody();

        std::string host = "127.0.0.1";
        if (body.contains("host") && body.at("host").is_string())
            host = std::string(body.at("host").as_string());

        boost::json::array globalIds;

        if (body.contains("instances") && body.at("instances").is_array())
        {
            for (const auto& inst : body.at("instances").as_array())
            {
                if (!inst.is_object()) continue;
                const auto& obj = inst.as_object();

                const uint32_t localId = obj.contains("localId")
                    ? static_cast<uint32_t>(obj.at("localId").as_int64()) : 0;
                const uint16_t port = obj.contains("port")
                    ? static_cast<uint16_t>(obj.at("port").as_int64()) : 0;

                const uint32_t globalId = nextGlobalId_++;

                auto server = RemoteGameServer::Create(this, globalId, host, port, client);
                gameServers_.push_back(server);
                replicaServers_[client].push_back(server);

                globalIds.push_back(boost::json::object{
                    {"localId",  static_cast<int64_t>(localId)},
                    {"globalId", static_cast<int64_t>(globalId)}
                });

                Log()->Msg("Replica registered: host={} port={} globalId={}", host, port, globalId);
            }
        }

        const uint64_t sourceJobId = msg->GetHeaders()->GetSourceJobID();
        client->Send("internal::register::response", {
            {"success",   true},
            {"globalIds", globalIds}
        }, sourceJobId);
    }

    void Controller::HandleStats(const Client::Shared& /*client*/, const Message::Shared& msg)
    {
        auto& body = msg->GetBody();
        if (!body.contains("instances") || !body.at("instances").is_array())
            return;

        for (const auto& inst : body.at("instances").as_array())
        {
            if (!inst.is_object()) continue;
            const auto& obj = inst.as_object();

            if (!obj.contains("globalId")) continue;
            const uint32_t globalId = static_cast<uint32_t>(obj.at("globalId").as_int64());

            uint32_t playerCount = 0;
            if (obj.contains("playerCount"))
                playerCount = static_cast<uint32_t>(obj.at("playerCount").as_int64());

            std::vector<std::pair<std::string, uint32_t>> leaderboard;
            if (obj.contains("leaderboard") && obj.at("leaderboard").is_array())
            {
                for (const auto& entry : obj.at("leaderboard").as_array())
                {
                    if (!entry.is_object()) continue;
                    const auto& eobj = entry.as_object();
                    std::string login = eobj.contains("login")
                        ? std::string(eobj.at("login").as_string()) : "";
                    uint32_t exp = eobj.contains("exp")
                        ? static_cast<uint32_t>(eobj.at("exp").as_int64()) : 0;
                    if (!login.empty())
                        leaderboard.emplace_back(login, exp);
                }
            }

            if (obj.contains("highScores") && obj.at("highScores").is_array())
            {
                for (const auto& entry : obj.at("highScores").as_array())
                {
                    if (!entry.is_object()) continue;
                    const auto& eobj = entry.as_object();
                    const std::string login = eobj.contains("login")
                        ? std::string(eobj.at("login").as_string()) : "";
                    const uint32_t exp = eobj.contains("exp")
                        ? static_cast<uint32_t>(eobj.at("exp").as_int64()) : 0;

                    if (!login.empty())
                    {
                        PersistPlayerScore(login, exp) = [this, login](const bool saved) {
                            if (!saved)
                                Log()->Warning("Failed to persist score for '{}'", login);
                        };
                    }
                }
            }

            for (const auto& server : gameServers_)
            {
                if (server->GetServerID() == globalId)
                {
                    server->UpdateStats(playerCount, std::move(leaderboard));
                    break;
                }
            }
        }
    }

    void Controller::HandleReplicaDisconnected(const Client::Shared& client)
    {
        if (!replicaServers_.contains(client))
            return;

        for (const auto& s : replicaServers_.at(client))
        {
            std::erase(gameServers_, s);
            Log()->Msg("Removed remote game server globalId={}", s->GetServerID());
        }
        replicaServers_.erase(client);
    }

    Utils::Task<bool> Controller::PersistPlayerScore(std::string login, uint32_t score)
    {
        if (!database_ || login.empty())
            co_return false;

        const auto result = co_await database_->Query(
            "update `snake_players` set `experience` = greatest(coalesce(`experience`, 0), {}) where `login` = {}",
            score,
            login);

        co_return result && result->IsSuccess();
    }

} // namespace Core::App::Game
