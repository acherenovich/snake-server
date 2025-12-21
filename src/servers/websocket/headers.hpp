#pragma once

#include "interfaces/server.hpp"

#include <utility>
#include <websocket.hpp>

#include <memory>
#include <string>

namespace Core::Servers::Websocket {
    namespace Net = Utils::Net::Websocket;

    class Headers final :
        public Interface::Headers,
        public std::enable_shared_from_this<Headers>
    {
        uint64_t sourceJobID = 0;
        uint64_t targetJobID = 0;
    public:
        using Shared    = std::shared_ptr<Headers>;

        explicit Headers(const boost::json::object & body)
        {
            const boost::json::value * v = nullptr;

            v = body.if_contains("sourceJobId");
            if (v != nullptr && v->is_int64()) {
                sourceJobID = v->as_int64();
            }

            v = body.if_contains("targetJobId");
            if (v != nullptr && v->is_int64()) {
                targetJobID = v->as_int64();
            }
        }

        uint64_t GetSourceJobID() const override
        {
            return sourceJobID;
        }

        uint64_t GetTargetJobID() const override
        {
            return targetJobID;
        }

    private:
        std::string GetServiceContainerName() const override
        {
            return "Headers";
        }

    public:
        static Shared Create(const BaseServiceContainer * parent, const boost::json::object &body)
        {
            const auto obj = std::make_shared<Headers>(body);
            obj->SetupContainer(parent);
            return obj;
        }
    };

} // namespace Core::Servers::Websocket