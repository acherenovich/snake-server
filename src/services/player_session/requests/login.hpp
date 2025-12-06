#pragma once

#include "[requests_loader].hpp"

namespace Core::App::PlayerSession::Requests {
    class Login: public RequestsServiceInstance
    {
        public:
        void Initialise() override;

        void Incoming(const Client::Shared & client, const Message::Shared & message) override;

        std::string GetType() override
        {
            return "player_session::login";
        };
    };
}
