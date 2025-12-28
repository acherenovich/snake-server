#pragma once

#include <unordered_set>

#include "[core_loader].hpp"

#include "legacy_logic.hpp"
#include "legacy_entities.hpp"

namespace Core::App::Game
{
    using Logic = Utils::Legacy::Game::Logic;
    using Snake = Utils::Legacy::Game::Interface::Entity::Snake;
    using Food = Utils::Legacy::Game::Interface::Entity::Food;

    namespace Interface {
        class GameServer: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<GameServer>;

            // virtual Snake::Shared GetPlayerSnake() = 0;
            //
            // virtual std::unordered_set<Snake::Shared> GetNearestVictims() = 0;
            //
            // virtual std::unordered_set<Food::Shared> GetNearestFoods() = 0;
        };
    }

}
