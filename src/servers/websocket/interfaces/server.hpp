#pragma once

#include "[core_loader].hpp"

#include <boost/json.hpp>

#include "coroutine.hpp"

namespace Core::Servers::Websocket {
    namespace Interface {
        class Headers: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<Headers>;

            ~Headers() override = default;

            [[nodiscard]] virtual uint64_t GetSourceJobID() const = 0;

            [[nodiscard]] virtual uint64_t GetTargetJobID() const = 0;
        };

        class Message: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<Message>;

            ~Message() override = default;

            [[nodiscard]] virtual Headers::Shared GetHeaders() const = 0;

            [[nodiscard]] virtual const std::string & GetType() const = 0;

            [[nodiscard]] virtual boost::json::object & GetBody() = 0;

            [[nodiscard]] virtual std::string ToString() const = 0;
        };

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
            using MessageCallback = std::function<void(const Message::Shared &)>;

            ~Client() override = default;

            [[nodiscard]] virtual bool IsConnected() const = 0;

            virtual uint64_t Send(const std::string & type, const boost::json::object & body, uint64_t targetJobID = 0) = 0;

            virtual Utils::Task<Message::Shared> Request(const std::string & type, const boost::json::object & body, uint64_t timeout = 5000) = 0;

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
