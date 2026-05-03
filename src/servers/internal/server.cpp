#include "servers/internal/server.hpp"
#include "utils.hpp"

#include <ranges>

namespace Core::Servers::Internal {

    void InternalServer::Initialise()
    {
        Net::ServerConfig config;
        config.address   = "0.0.0.0";
        config.port      = Utils::EnvInt("INTERNAL_WS_PORT", 9101);
        config.mode      = Net::Mode::Json;
        config.ioThreads = 2;
        config.useTls    = false;

        server_ = Net::Server::Create(config, shared_from_this(), Log());

        Log()->Msg("Internal server created on {}:{}", config.address, config.port);
    }

    void InternalServer::OnAllServicesLoaded()
    {
        IFace().Register<Interface::Server>(shared_from_this());
    }

    void InternalServer::OnAllInterfacesLoaded() {}

    void InternalServer::ProcessTick()
    {
        server_->ProcessTick();

        for (const auto& client : clients_ | std::views::values)
            client->ProcessTick();
    }

    void InternalServer::OnSessionConnected(const Net::Session::Shared& session)
    {
        Log()->Msg("Game-server replica connected: {}:{}", session->RemoteAddress(), session->RemotePort());

        const auto client = CreateClient(session);
        for (const auto& handler : clientHandlers_)
            handler(client, WsClient::Events::ClientConnected);
    }

    void InternalServer::OnSessionDisconnected(const Net::Session::Shared& session)
    {
        Log()->Msg("Game-server replica disconnected: {}:{}", session->RemoteAddress(), session->RemotePort());

        const auto client = CloseClient(session);
        client->Disconnected();
        for (const auto& handler : clientHandlers_)
            handler(client, WsClient::Events::ClientDisconnected);
    }

    void InternalServer::OnMessage(const Net::Session::Shared& session, const boost::json::value& jsonValue)
    {
        if (!jsonValue.is_object())
            return;

        const auto& obj = jsonValue.as_object();

        const auto* headersValue = obj.if_contains("headers");
        if (headersValue == nullptr || !headersValue->is_object())
            return;

        const auto headers = WsHeaders::Create(this, headersValue->as_object());

        const auto* typeValue = obj.if_contains("type");
        if (typeValue == nullptr || !typeValue->is_string())
            return;

        const std::string type(typeValue->as_string());

        const auto* messageValue = obj.if_contains("message");
        if (messageValue == nullptr || !messageValue->is_object())
            return;

        const auto message = WsMessage::Create(this, headers, type, messageValue->get_object());
        const auto client  = GetClient(session);
        client->OnMessage(message);

        if (const auto it = messageHandlers_.find(type); it != messageHandlers_.end())
            for (const auto& handler : it->second)
                handler(client, message);
    }

    void InternalServer::RegisterMessage(const std::string& type, const MessageCallback& callback)
    {
        messageHandlers_[type].push_back(callback);
    }

    void InternalServer::RegisterClientsCallback(const ClientCallback& callback)
    {
        clientHandlers_.push_back(callback);
    }

    WsClient::Shared InternalServer::GetClient(const Net::Session::Shared& session)
    {
        if (const auto it = clients_.find(session); it != clients_.end())
            return it->second;
        Log()->Fatal("GetClient: session not found");
        return {};
    }

    WsClient::Shared InternalServer::CreateClient(const Net::Session::Shared& session)
    {
        return clients_[session] = WsClient::Create(this, session);
    }

    WsClient::Shared InternalServer::CloseClient(const Net::Session::Shared& session)
    {
        const auto it = clients_.find(session);
        if (it == clients_.end())
        {
            Log()->Fatal("CloseClient: session not found");
            return {};
        }

        const auto client = it->second;
        client->Close();
        clients_.erase(it);
        return client;
    }

} // namespace Core::Servers::Internal
