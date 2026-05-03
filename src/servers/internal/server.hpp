#pragma once

#include "interfaces/server.hpp"
#include "servers/websocket/client.hpp"
#include "servers/websocket/message.hpp"
#include "servers/websocket/headers.hpp"

#include <websocket.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Servers::Internal {

    namespace Net = Utils::Net::Websocket;

    using WsClient  = Core::Servers::Websocket::Client;
    using WsMessage = Core::Servers::Websocket::Message;
    using WsHeaders = Core::Servers::Websocket::Headers;

    class InternalServer final :
        public BaseServiceInstance,
        public Net::Listener,
        public Interface::Server,
        public std::enable_shared_from_this<InternalServer>
    {
        Net::Server::Shared server_;
        std::unordered_map<std::string, std::vector<MessageCallback>> messageHandlers_;
        std::vector<ClientCallback> clientHandlers_;
        std::unordered_map<Net::Session::Shared, WsClient::Shared> clients_;

    public:
        using Shared = std::shared_ptr<InternalServer>;

        void Initialise() override;
        void OnAllServicesLoaded() override;
        void OnAllInterfacesLoaded() override;
        void ProcessTick() override;

        void OnSessionConnected(const Net::Session::Shared& session) override;
        void OnSessionDisconnected(const Net::Session::Shared& session) override;
        void OnMessage(const Net::Session::Shared& session, const boost::json::value& jsonValue) override;

        void RegisterMessage(const std::string& type, const MessageCallback& callback) override;
        void RegisterClientsCallback(const ClientCallback& callback) override;

        std::string GetServiceContainerName() const override { return "NET-INTERNAL"; }

    private:
        WsClient::Shared GetClient(const Net::Session::Shared& session);
        WsClient::Shared CreateClient(const Net::Session::Shared& session);
        WsClient::Shared CloseClient(const Net::Session::Shared& session);
    };

} // namespace Core::Servers::Internal
