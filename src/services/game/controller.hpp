#pragma once

#include "interfaces/controller.hpp"
#include "servers/websocket/interfaces/server.hpp"
#include "servers/internal/interfaces/server.hpp"
#include "remote_game_server.hpp"

#include <unordered_map>

namespace Core::App::Game
{
    // Управляет реестром удалённых игровых серверов.
    // Реплики snake-game-server подключаются по внутреннему WebSocket (порт 9101),
    // регистрируют свои экземпляры и периодически обновляют статистику.
    class Controller final :
        public Interface::Controller,
        public std::enable_shared_from_this<Controller>
    {
        using WsServer  = Servers::Websocket::Interface::Server;
        using IntServer = Servers::Internal::Interface::Server;
        using Client    = Servers::Websocket::Interface::Client;
        using Message   = Servers::Websocket::Interface::Message;

        WsServer::Shared  websocket_;
        IntServer::Shared internalServer_;

        std::vector<RemoteGameServer::Shared> gameServers_;
        std::unordered_map<Client::Shared, std::vector<RemoteGameServer::Shared>> replicaServers_;

        uint32_t nextGlobalId_ { 1 };

    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;
        void OnAllServicesLoaded() override;
        void OnAllInterfacesLoaded() override;
        void ProcessTick() override;

        [[nodiscard]] std::map<uint32_t, Interface::GameServer::Shared> GetGameServers() const override;

        std::string GetServiceContainerName() const override { return "Game"; }

    private:
        void HandleRegister(const Client::Shared& client, const Message::Shared& msg);
        void HandleStats(const Client::Shared& client, const Message::Shared& msg);
        void HandleReplicaDisconnected(const Client::Shared& client);
    };

} // namespace Core::App::Game
