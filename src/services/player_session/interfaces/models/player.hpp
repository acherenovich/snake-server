#pragma once

#include "[base].hpp"
#include "coroutine.hpp"

namespace Core::App::PlayerSession::Model
{
    namespace Interface
    {
        class Player: public virtual BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<Player>;

            [[nodiscard]] virtual PlayerType GetPlayerType() const = 0;

            virtual Utils::Task<bool> Login(std::string login, std::string password) = 0;

            virtual Utils::Task<bool> Login(std::string token) = 0;
        };
    }
}