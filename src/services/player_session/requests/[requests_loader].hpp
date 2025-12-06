#pragma once

#include "servers/websocket/interfaces/server.hpp"

#include "services/player_session/interfaces/controller.hpp"
#include "[core_loader].hpp"

namespace Core::App::PlayerSession::Requests {

    class RequestsServiceContainer: public BaseServiceContainer
    {
        Interface::Controller::Shared controller_ {};

    public:
        using Shared = std::shared_ptr<RequestsServiceContainer>;

        explicit RequestsServiceContainer(const Interface::Controller::Shared & controller): controller_(controller) {}
        explicit RequestsServiceContainer() {}

        ~RequestsServiceContainer() override = default;

        void SetupContainer(const RequestsServiceContainer & parent)
        {
            controller_ = parent.controller_;

            BaseServiceContainer::SetupContainer(parent);
        }

        void SetupContainer(const RequestsServiceContainer * parent)
        {
            SetupContainer(*parent);
        }
    };

    class RequestsServiceInstance : public Utils::Service::Instance, public RequestsServiceContainer {
    protected:
        using Server = Servers::Websocket::Interface::Server;
        using Client = Servers::Websocket::Interface::Client;
        using Message = Servers::Websocket::Interface::Message;

        Server::Shared server_ {};
    public:
        using Shared = std::shared_ptr<RequestsServiceInstance>;

        explicit RequestsServiceInstance() {}

        ~RequestsServiceInstance() override = default;

        void OnAllInterfacesLoaded() override
        {
            server_ = IFace().Get<Server>();

            server_->RegisterMessage(GetType(), [this](const Client::Shared & client, const Message::Shared & message) {
                Incoming(client, message);
            });
        }

        virtual void Incoming(const Client::Shared & client, const Message::Shared & message) = 0;

        virtual std::string GetType() = 0;
    };

    using Loader = Utils::Service::Loader;

    inline Loader & RequestsLoader()
    {
        static Loader loader;
        return loader;
    }
} // namespace Core