#pragma once

#include "interfaces/server.hpp"
#include "headers.hpp"

#include <utility>
#include <websocket.hpp>

#include <memory>
#include <string>

namespace Core::Servers::Websocket {
    namespace Net = Utils::Net::Websocket;

    class Message final :
        public Interface::Message,
        public std::enable_shared_from_this<Message>
    {
        Headers::Shared headers_;
        std::string type_;
        boost::json::object body_;
    public:
        using Shared    = std::shared_ptr<Message>;

        Message(Headers::Shared headers, std::string type, boost::json::object body):
        headers_(std::move(headers)), type_(std::move(type)), body_(std::move(body)) {}

        Interface::Headers::Shared GetHeaders() const override
        {
            return headers_;
        }

        const std::string & GetType() const override
        {
            return type_;
        }

        boost::json::object & GetBody() override
        {
            return body_;
        }

        std::string ToString() const override
        {
            return boost::json::serialize(body_);
        }

    private:
        std::string GetServiceContainerName() const override
        {
            return type_;
        }

    public:
        static Shared Create(const BaseServiceContainer * parent, const Headers::Shared & headers, const std::string & type, const boost::json::object &body)
        {
            const auto obj = std::make_shared<Message>(headers, type, body);
            obj->SetupContainer(parent);
            return obj;
        }

        static Shared Impl(const Interface::Message::Shared & session)
        {
            if (!session)
                return {};
            return std::dynamic_pointer_cast<Message>(session);
        }
    };

} // namespace Core::Servers::Websocket