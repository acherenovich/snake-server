#pragma once

#include "common.hpp"
#include "game_server.hpp"

#include "coroutine.hpp"

namespace Core::App::Game
{
    namespace Interface
    {
        class Controller: public BaseServiceInterface, public BaseServiceInstance
        {
        public:
            using Shared = std::shared_ptr<Controller>;

            // [[nodiscard]] virtual GameClient::Shared GetCurrentGameClient() const = 0;
        };
    }
}
