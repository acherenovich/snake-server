#include "remote_game_server.hpp"

#include <format>

namespace Core::App::Game
{
    RemoteGameServer::RemoteGameServer(uint32_t globalId,
                                       std::string host,
                                       uint16_t port,
                                       Client::Shared client)
        : globalId_(globalId)
        , host_(std::move(host))
        , port_(port)
        , replicaClient_(std::move(client))
    {}

    uint32_t RemoteGameServer::GetServerID() const
    {
        return globalId_;
    }

    uint32_t RemoteGameServer::GetPlayersCount() const
    {
        return playerCount_;
    }

    std::string RemoteGameServer::GetHost() const
    {
        return host_;
    }

    uint16_t RemoteGameServer::GetPort() const
    {
        return port_;
    }

    void RemoteGameServer::SetSSIDPlayer(uint64_t ssid, const Player::Shared& player)
    {
        // Fire-and-forget: сообщаем реплике связать UDP-сессию с игроком
        replicaClient_->Send("internal::connect_udp", {
            {"globalId",    static_cast<int64_t>(globalId_)},
            {"ssid",        static_cast<int64_t>(ssid)},
            {"playerLogin", player->Model()->GetLogin()}
        });
    }

    std::vector<std::pair<std::string, uint32_t>> RemoteGameServer::GetLeaderboard()
    {
        return leaderboard_;
    }

    void RemoteGameServer::UpdateStats(uint32_t playerCount,
                                       std::vector<std::pair<std::string, uint32_t>> leaderboard)
    {
        playerCount_ = playerCount;
        leaderboard_ = std::move(leaderboard);
    }

    RemoteGameServer::Shared RemoteGameServer::Create(const BaseServiceContainer* parent,
                                                      uint32_t globalId,
                                                      const std::string& host,
                                                      uint16_t port,
                                                      const Client::Shared& client)
    {
        const auto obj = std::make_shared<RemoteGameServer>(globalId, host, port, client);
        obj->SetupContainer(parent);
        return obj;
    }

    std::string RemoteGameServer::GetServiceContainerName() const
    {
        return std::format("RemoteGameServer[{}]", globalId_);
    }

} // namespace Core::App::Game
