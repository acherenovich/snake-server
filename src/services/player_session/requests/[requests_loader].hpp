#pragma once

#include <utility>

#include "servers/websocket/interfaces/server.hpp"

#include "services/player_session/interfaces/controller.hpp"
#include "services/player_session/interfaces/player.hpp"
#include "[core_loader].hpp"

namespace Core::App::PlayerSession::Requests {

    class RequestsServiceContainer: public BaseServiceContainer
    {
        Interface::Controller::Shared controller_ {};

    public:
        using Shared = std::shared_ptr<RequestsServiceContainer>;

        explicit RequestsServiceContainer(Interface::Controller::Shared controller): controller_(std::move(controller)) {}
        explicit RequestsServiceContainer() = default;

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

        [[nodiscard]] const Interface::Controller::Shared & GetController() const
        {
            return controller_;
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

        explicit RequestsServiceInstance() = default;

        ~RequestsServiceInstance() override = default;

        void OnAllInterfacesLoaded() override
        {
            server_ = IFace().Get<Server>();

            server_->RegisterMessage(GetType(), [this](const Client::Shared & client, const Message::Shared & message) {
                const auto player = GetController()->GetPlayer(client);
                if (!player)
                    return;

                try
                {
                    Incoming(player, message);
                }
                catch (const std::exception & e)
                {
                    Log()->Error("exception: {}", e.what());
                    SendFail(player, "internal_server_error");
                }
            });
        }

        uint64_t SendResponse(const Interface::Player::Shared & player, const boost::json::object & message, const uint64_t targetJobID = 0)
        {
            return player->GetClient()->Send(GetType() + "::response", message, targetJobID);
        }

        void SendFail(const Interface::Player::Shared & player, const std::string & reason, const uint64_t targetJobID = 0)
        {
            SendResponse(player, {{"success", false}, {"message", reason}}, targetJobID);
        }

        void SendSuccess(const Interface::Player::Shared & player, const boost::json::object & message, const uint64_t targetJobID = 0)
        {
            SendResponse(player, {{"success", true}, {"body", message}}, targetJobID);
        }

        virtual void Incoming(const Interface::Player::Shared & player, const Message::Shared & message) = 0;

        [[nodiscard]] virtual std::string GetType() const = 0;

        [[nodiscard]] std::string GetServiceContainerName() const override
        {
            return GetType();
        }
    };

    using Loader = Utils::Service::Loader;

    inline Loader & RequestsLoader()
    {
        static Loader loader;
        return loader;
    }
} // namespace Core