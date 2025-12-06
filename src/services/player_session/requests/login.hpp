#pragma once

#include "[requests_loader].hpp"

namespace Core::App::PlayerSession::Requests {
    class Login: public RequestsServiceInstance
    {
        public:
        void Initialise() override;

        void Incoming(const Interface::Player::Shared & player, const Message::Shared & message) override;

        [[nodiscard]] std::string GetType() const override
        {
            return "player_session::login";
        }
    };
}
