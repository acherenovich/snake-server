#pragma once

#include "interfaces/server.hpp"

#include <utility>
#include <websocket.hpp>

#include <memory>
#include <string>
#include <unordered_map>

#include "message.hpp"

namespace Core::Servers::Websocket {
    namespace Net = Utils::Net::Websocket;

    class Client final :
        public Interface::Client,
        public std::enable_shared_from_this<Client>
    {
        Net::Session::Shared session_;
        bool connected_ = true;

        struct JobHandler
        {
            std::chrono::steady_clock::time_point expireAt;
            MessageCallback callback;
        };
        std::unordered_map<uint64_t, JobHandler> jobsHandlers_;

        uint64_t sourceJobID_ = 1;
    public:
        using Shared    = std::shared_ptr<Client>;

        explicit Client(Net::Session::Shared session): session_(std::move(session)) {}

        bool IsConnected() const override
        {
            return connected_;
        }

        void OnMessage(const Message::Shared & message)
        {
            if (const auto targetJobID = message->GetHeaders()->GetTargetJobID(); jobsHandlers_.contains(targetJobID))
            {
                jobsHandlers_[targetJobID].callback(message);
                jobsHandlers_.erase(targetJobID);
            }
        }

        uint64_t Send(const std::string & type, const boost::json::object & body, uint64_t targetJobID = 0) override
        {
            if (!IsConnected())
            {
                Log()->Warning("Send in disconnected state!");
                return 0;
            }

            const auto sourceJobID = sourceJobID_++;

            boost::json::object headers = {
                {"sourceJobId", sourceJobID},
                {"targetJobId", targetJobID},
            };

            boost::json::object message = {
                {"type", type},
                {"headers", headers},
                {"message", body},
            };

            session_->Send(message);

            return sourceJobID;
        }

        Utils::Task<Interface::Message::Shared> Request(const std::string & type, const boost::json::object & body, const uint64_t timeout = 5000) override
        {
            if (!IsConnected())
            {
                Log()->Warning("Request in disconnected state!");
                co_return {};
            }

            const auto sourceJobID = Send(type, body);

            Message::Shared response;

            co_await Utils::AwaitablePromiseTask([=, this, &response](const Utils::TaskResolver & resolver)  {
                RegisterJobCallback(sourceJobID, [=, &response](const Interface::Message::Shared & message) {
                    response = Message::Impl(message);
                    resolver->Resolve();
                }, timeout);
            });

            if (!response)
            {
                Log()->Error("Request '{}' timeout", type);
                co_return {};
            }

            co_return response;
        }

        void RegisterJobCallback(const uint64_t jobID, const MessageCallback & callback, const uint64_t timeout)
        {
            const auto now = std::chrono::steady_clock::now();

            jobsHandlers_[jobID] = JobHandler{
                .expireAt = now + std::chrono::milliseconds{ timeout },
                .callback = callback
            };
        }

        void Close()
        {
            ClearJobHandlers();
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

        void ProcessTick()
        {
            const auto now = std::chrono::steady_clock::now();
            ClearJobHandlers(now);
        }

        void ClearJobHandlers(const std::chrono::steady_clock::time_point & time = std::chrono::steady_clock::time_point::max())
        {
            for (auto it = jobsHandlers_.begin(); it != jobsHandlers_.end(); )
            {
                if (it->second.expireAt <= time)
                {
                    auto callback = std::move(it->second.callback);

                    it = jobsHandlers_.erase(it);

                    callback({});
                }
                else
                {
                    ++it;
                }
            }
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