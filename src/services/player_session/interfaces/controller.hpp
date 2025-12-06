#pragma once

#include "player.hpp"
#include "servers/websocket/message.hpp"

namespace Core::App::PlayerSession
{
    namespace Interface
    {
        class Controller: public virtual BaseServiceInstance
        {
        public:
            using Shared = std::shared_ptr<Controller>;

            virtual void CreateSession(uint32_t ownerAccountID) = 0;

            virtual Player::Shared GetPlayer(const Servers::Websocket::Interface::Client::Shared & client) = 0;
        };
    }
}
