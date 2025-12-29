#pragma once

#include "interfaces/controller.hpp"

#include "servers/websocket/interfaces/server.hpp"

#include "game_server.hpp"

namespace Core::App::Game
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
        using Server = Servers::Websocket::Interface::Server;
        using Message = Servers::Websocket::Interface::Message;
        using Client = Servers::Websocket::Interface::Client;

        Server::Shared websocket_;
        std::vector<GameServer::Shared> gameServers_;
    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;

        void OnAllServicesLoaded() override;

        void OnAllInterfacesLoaded() override;

        void ProcessTick() override;

        [[nodiscard]] std::vector<Interface::GameServer::Shared> GetGameServers() const override;

    private:
        std::string GetServiceContainerName() const override
        {
            return "Game";
        }
    };
}