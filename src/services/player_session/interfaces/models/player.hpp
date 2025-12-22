#pragma once

#include <boost/json/object.hpp>

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

            virtual Utils::Task<bool> Register(std::string login, std::string password) = 0;

            virtual Utils::Task<bool> Login(std::string login, std::string password) = 0;

            virtual Utils::Task<bool> Login(std::string token) = 0;

            enum SerialiseType
            {
                SerialisePlayer,
                SerialiseToken,
            };
            [[nodiscard]] virtual boost::json::object Serialise(const SerialiseType & serialiseType = SerialisePlayer) = 0;

        };
    }
}