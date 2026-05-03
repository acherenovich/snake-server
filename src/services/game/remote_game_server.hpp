#pragma once

#include "interfaces/game_server.hpp"
#include "servers/websocket/interfaces/server.hpp"

namespace Core::App::Game
{
    // Прокси-объект: представляет игровой сервер на удалённой реплике snake-game-server.
    // Данные (playerCount, leaderboard) кэшируются из stats-апдейтов.
    class RemoteGameServer final :
        public Interface::GameServer,
        public std::enable_shared_from_this<RemoteGameServer>
    {
        uint32_t globalId_;
        std::string host_;
        uint16_t port_;

        uint32_t playerCount_ { 0 };
        std::vector<std::pair<std::string, uint32_t>> leaderboard_;

        using Client = Servers::Websocket::Interface::Client;
        Client::Shared replicaClient_;

    public:
        using Shared = std::shared_ptr<RemoteGameServer>;

        RemoteGameServer(uint32_t globalId,
                         std::string host,
                         uint16_t port,
                         Client::Shared client);

        [[nodiscard]] uint32_t GetServerID()    const override;
        [[nodiscard]] uint32_t GetPlayersCount() const override;
        [[nodiscard]] std::string GetHost()     const override;
        [[nodiscard]] uint16_t GetPort()        const override;

        void SetSSIDPlayer(uint64_t ssid, const Player::Shared& player) override;

        std::vector<std::pair<std::string, uint32_t>> GetLeaderboard() override;

        void UpdateStats(uint32_t playerCount,
                         std::vector<std::pair<std::string, uint32_t>> leaderboard);

        static Shared Create(const BaseServiceContainer* parent,
                             uint32_t globalId,
                             const std::string& host,
                             uint16_t port,
                             const Client::Shared& client);

    private:
        std::string GetServiceContainerName() const override;
    };

} // namespace Core::App::Game
