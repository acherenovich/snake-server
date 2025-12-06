#pragma once

#include "[core_loader].hpp"

#include <boost/json.hpp>

namespace Core::Servers::Websocket {
    namespace Interface {
        class Client: public BaseServiceContainer
        {
        public:
            enum Events
            {
                ClientConnected,
                ClientDisconnected,
            };
        public:
            using Shared = std::shared_ptr<Client>;

            virtual ~Client() = default;

            [[nodiscard]] virtual bool IsConnected() const = 0;
        };

        class Message: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<Message>;

            virtual ~Message() = default;

            [[nodiscard]] virtual const std::string & GetType() const = 0;

            [[nodiscard]] virtual boost::json::object & GetBody() = 0;

            [[nodiscard]] virtual std::string ToString() const = 0;
        };

        class Server: public BaseServiceInterface
        {
        public:
            using Shared = std::shared_ptr<Server>;

            virtual ~Server() = default;

            using MessageCallback = std::function<void(const Client::Shared &, const Message::Shared &)>;
            virtual void RegisterMessage(const std::string & type, const MessageCallback & callback) = 0;

            using ClientCallback = std::function<void(const Client::Shared &, const Client::Events &)>;
            virtual void RegisterClientsCallback(const ClientCallback & callback) = 0;
        };
    }
}
