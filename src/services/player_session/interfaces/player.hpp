#pragma once

#include "common.hpp"

#include "models/player.hpp"

#include "servers/websocket/interfaces/server.hpp"

namespace Core::App::PlayerSession
{
    namespace Interface
    {
        class Player: public virtual BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<Player>;

            [[nodiscard]] virtual Model::Interface::Player::Shared Model() = 0;

            [[nodiscard]] const virtual Servers::Websocket::Interface::Client::Shared & GetClient() const = 0;
        };
    }
}