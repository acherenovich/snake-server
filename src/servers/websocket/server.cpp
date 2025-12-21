#include "servers/websocket/server.hpp"

#include <chrono>
#include <ranges>

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

        const auto now = std::chrono::steady_clock::now();

        for (const auto &client: clients_ | std::views::values)
        {
            client->ProcessTick();
        }
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
        session->Log()->Debug("Message: {}", serialize(jsonValue));

        if (!jsonValue.is_object()) {
            Log()->Warning("Incoming JSON is not an object");
            return;
        }

        const boost::json::object & obj = jsonValue.as_object();

        const boost::json::value * headersValue = obj.if_contains("headers");
        if (headersValue == nullptr || !headersValue->is_object()) {
            Log()->Warning("Incoming JSON has no string 'headers' field");
            return;
        }

        const auto headers = Headers::Create(this, headersValue->as_object());

        const boost::json::value * typeValue = obj.if_contains("type");
        if (typeValue == nullptr || !typeValue->is_string()) {
            Log()->Warning("Incoming JSON has no string 'type' field");
            return;
        }

        const std::string type = std::string(typeValue->as_string());

        const boost::json::value * messageValue = obj.if_contains("message");
        if (messageValue == nullptr || !messageValue->is_object()) {
            Log()->Warning("Incoming JSON has no object 'message' field");
            return;
        }

        const auto message = Message::Create(this, headers, type, messageValue->get_object());
        const auto client = GetClient(session);
        client->OnMessage(message);

        if (const auto it = messageHandlers_.find(type); it != messageHandlers_.end()) {
            for (const auto & handler : it->second)
                handler(client, message);
        }
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