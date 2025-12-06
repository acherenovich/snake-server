#pragma once

#include "interfaces/controller.hpp"
#include "requests/[requests_loader].hpp"

#include "player.hpp"

namespace Core::App::PlayerSession
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
        using Server = Servers::Websocket::Interface::Server;
        using Client = Servers::Websocket::Interface::Client;
        using Message = Servers::Websocket::Interface::Message;

        Server::Shared server_ {};

        std::unordered_map<Client::Shared, Player::Shared> players_ {};
    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;

        void OnAllInterfacesLoaded() override;

        void CreateSession(uint32_t ownerAccountID) override;

        Interface::Player::Shared GetPlayer(const Servers::Websocket::Interface::Client::Shared & client) override;

        void OnClientConnected(const Client::Shared & client);

        void OnClientDisconnected(const Client::Shared & client);




        std::string GetServiceContainerName() const override
        {
            return "PL_SES";
        }

        std::vector<Utils::Service::SubLoader> SubLoaders() override
        {
            static Requests::RequestsServiceContainer container{shared_from_this()};

            return
            {
                {
                    Requests::RequestsLoader(),
                    [](auto child_) {
                        const auto child = dynamic_cast<Requests::RequestsServiceContainer*>(child_);
                        child->SetupContainer(container);
                    }
                }
            };
        };
    };

}