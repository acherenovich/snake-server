#include "servers/websocket/server.hpp"

#include <chrono>

namespace Core::Servers::Websocket {

    using namespace std::chrono_literals;

    void WebsocketService::Initialise()
    {
        Net::ServerConfig config;
        config.address   = "0.0.0.0";
        config.port      = 9100;
        config.mode      = Net::Mode::Json;
        config.ioThreads = 4;
        config.useTls    = false;

        server_ = Net::Server::Create(config, shared_from_this(), Log());

        Log()->Debug("Server created on {}:{}", config.address, config.port);
    }

    void WebsocketService::OnAllServicesLoaded()
    {
        Log()->Debug("OnAllServicesLoaded()");

        IFace().Register<Interface::Server>(shared_from_this());
    }

    void WebsocketService::OnAllInterfacesLoaded()
    {
        Log()->Debug("OnAllInterfacesLoaded()");
    }

    void WebsocketService::ProcessTick()
    {
        server_->ProcessTick();
    }

    void WebsocketService::OnSessionConnected(const Net::Session::Shared & session)
    {
        Log()->Debug("Client connected: {}:{}",
                        session->RemoteAddress(),
                        session->RemotePort());

        const auto client = CreateClient(session);
        for (const auto & handler : clientHandlers_)
            handler(client, Client::Events::ClientConnected);
    }

    void WebsocketService::OnSessionDisconnected(const Net::Session::Shared & session)
    {
        Log()->Debug("Client disconnected: {}:{}",
                        session->RemoteAddress(),
                        session->RemotePort());

        const auto client = CloseClient(session);
        client->Disconnected();
        for (const auto & handler : clientHandlers_)
            handler(client, Client::Events::ClientDisconnected);
    }

    void WebsocketService::OnMessage(const Net::Session::Shared & session,
                                         const boost::json::value & jsonValue)
    {
        if (!jsonValue.is_object()) {
            Log()->Warning("Incoming JSON is not an object");
            return;
        }

        const boost::json::object & obj = jsonValue.as_object();

        const boost::json::value * typeValue = obj.if_contains("type");
        if (typeValue == nullptr || !typeValue->is_string()) {
            Log()->Warning("Incoming JSON has no string 'type' field");
            return;
        }

        const std::string type = std::string(typeValue->as_string());

        const auto it = messageHandlers_.find(type);
        if (it == messageHandlers_.end()) {
            Log()->Warning("No handler registered for type '{}'", type);
            return;
        }

        const boost::json::value * messageValue = obj.if_contains("message");
        if (messageValue == nullptr || !messageValue->is_object()) {
            Log()->Warning("Incoming JSON has no object 'message' field");
            return;
        }

        const auto client = GetClient(session);
        const auto message = Message::Create(this, type, messageValue->get_object());

        for (const auto & handler : it->second)
            handler(client, message);
    }

    void WebsocketService::RegisterMessage(const std::string & type, const MessageCallback & callback)
    {
        messageHandlers_[type].push_back(callback);
    }

    void WebsocketService::RegisterClientsCallback(const ClientCallback & callback)
    {
        clientHandlers_.push_back(callback);
    }

} // namespace Core::Servers::Websocket