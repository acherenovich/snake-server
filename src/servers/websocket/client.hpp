#pragma once

#include "interfaces/server.hpp"

#include <utility>
#include <websocket.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Servers::Websocket {
    namespace Net = Utils::Net::Websocket;

    class Client final :
        public Interface::Client,
        public std::enable_shared_from_this<Client>
    {
        Net::Session::Shared session_;
        bool connected_ = true;
    public:
        using Shared    = std::shared_ptr<Client>;

        explicit Client(Net::Session::Shared session): session_(std::move(session)) {}

        bool IsConnected() const override
        {
            return connected_;
        }

        void Disconnected()
        {
            connected_ = false;
        }

        void Disconnect()
        {
            connected_ = false;
            session_->Close();
        }

    private:
        std::string GetServiceContainerName() const override
        {
            return std::format("{}:{}", session_->RemoteAddress(), session_->RemotePort());
        }

    public:
        static Shared Create(const BaseServiceContainer * parent, const Net::Session::Shared & session)
        {
            const auto obj = std::make_shared<Client>(session);
            obj->SetupContainer(parent);
            return obj;
        }
    };

} // namespace Core::Servers::Websocket