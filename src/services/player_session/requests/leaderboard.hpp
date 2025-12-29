#pragma once

#include "[requests_loader].hpp"

#include "services/game/interfaces/controller.hpp"

namespace Core::App::PlayerSession::Requests {
    using GameController = Game::Interface::Controller;

    class LeaderBoard final : public RequestsServiceInstance
    {
        GameController::Shared gameController_;
    public:
        void Initialise() override;

        void OnAllInterfacesLoadedPost() override;

        void Incoming(const Interface::Player::Shared & player, const Message::Shared & message) override;

        [[nodiscard]] std::string GetType() const override
        {
            return "player_session::leaderboard";
        }
    };
}
