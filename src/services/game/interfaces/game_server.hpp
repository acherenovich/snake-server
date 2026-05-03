#pragma once

#include <unordered_set>
#include <vector>
#include <utility>

#include "[core_loader].hpp"

#include "services/player_session/interfaces/player.hpp"

namespace Core::App::Game
{
    using Player = PlayerSession::Interface::Player;

    namespace Interface {
        class GameServer: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<GameServer>;

            [[nodiscard]] virtual uint32_t GetServerID() const = 0;

            [[nodiscard]] virtual uint32_t GetPlayersCount() const = 0;

            [[nodiscard]] virtual std::string GetHost() const = 0;

            [[nodiscard]] virtual uint16_t GetPort() const = 0;

            virtual void SetSSIDPlayer(uint64_t ssid, const Player::Shared & player) = 0;

            virtual std::vector<std::pair<std::string, uint32_t>> GetLeaderboard() = 0;
        };
    }

}
