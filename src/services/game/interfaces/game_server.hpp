#pragma once

#include <unordered_set>

#include "[core_loader].hpp"

#include "legacy_logic.hpp"
#include "legacy_entities.hpp"

#include "services/player_session/interfaces/player.hpp"

namespace Core::App::Game
{
    using Logic = Utils::Legacy::Game::Logic;
    using Snake = Utils::Legacy::Game::Interface::Entity::Snake;
    using Food = Utils::Legacy::Game::Interface::Entity::Food;

    using Player = PlayerSession::Interface::Player;

    namespace Interface {
        class GameServer: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<GameServer>;

            [[nodiscard]] virtual uint32_t GetServerID() const = 0;

            [[nodiscard]] virtual uint32_t GetPlayersCount() const = 0;

            virtual void SetSSIDPlayer(uint64_t ssid, const Player::Shared & player) = 0;

            virtual std::unordered_map<Player::Shared, uint32_t> GetLeaderboard() = 0;

            // virtual Snake::Shared GetPlayerSnake() = 0;
            //
            // virtual std::unordered_set<Snake::Shared> GetNearestVictims() = 0;
            //
            // virtual std::unordered_set<Food::Shared> GetNearestFoods() = 0;
        };
    }

}
