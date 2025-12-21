#pragma once

#include "interfaces/server.hpp"
#include "client.hpp"
#include "message.hpp"

#include <websocket.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Servers::Websocket {
    namespace Net = Utils::Net::Websocket;

    class WebsocketService;

    class WebsocketService final :
        public BaseServiceInstance,
        public Net::Listener,
        public Interface::Server,
        public std::enable_shared_from_this<WebsocketService>
    {
        Net::Server::Shared server_;
        std::unordered_map<std::string, std::vector<MessageCallback>> messageHandlers_;
        std::vector<ClientCallback> clientHandlers_;

        std::unordered_map<Net::Session::Shared, Client::Shared> clients_;
    public:
        using Shared    = std::shared_ptr<WebsocketService>;

        WebsocketService() = default;
        ~WebsocketService() override = default;

    protected:
        void Initialise() override;
        void OnAllServicesLoaded() override;
        void OnAllInterfacesLoaded() override;
        void ProcessTick() override;
    public:
        void OnSessionConnected(const Net::Session::Shared & session) override;
        void OnSessionDisconnected(const Net::Session::Shared & session) override;
        void OnMessage(const Net::Session::Shared & session, const boost::json::value & jsonValue) override;

        void RegisterMessage(const std::string & type, const MessageCallback & callback) override;

        void RegisterClientsCallback(const ClientCallback & callback) override;

        [[nodiscard]] Net::Server::Shared & Server()
        {
            return server_;
        }

    private:

        std::string GetServiceContainerName() const override
        {
            return "NET-WS";
        }

        Client::Shared GetClient(const Net::Session::Shared & session)
        {
            if (const auto it = clients_.find(session); it != clients_.end())
                return it->second;

            Log()->Fatal("GetClient: Client {} not found", session->RemoteAddress());
            return {};
        }

        Client::Shared CreateClient(const Net::Session::Shared & session)
        {
            if (const auto it = clients_.find(session); it != clients_.end())
                Log()->Fatal("CreateClient: Client {} already exists", session->RemoteAddress());

            return clients_[session] = Client::Create(this, session);
        }

        Client::Shared CloseClient(const Net::Session::Shared & session)
        {
            const auto it = clients_.find(session);
            if (it == clients_.end())
                Log()->Fatal("CloseClient: Client {} not found", session->RemoteAddress());

            const auto client = it->second;
            client->Close();
            clients_.erase(it);
            return client;
        }
    };

} // namespace Core::Servers::Websocket