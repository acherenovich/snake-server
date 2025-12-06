#include "login.hpp"

/*

{
    "login": "login",
    "password": "password",
    "token": "ABABABABBABABBA"
}

{
    "success": true,
    "token": "ABABABABBABABBA",
    "message": "user_not_found"
}

*/

namespace Core::App::PlayerSession::Requests {

    [[maybe_unused]] Utils::Service::Loader::Add<Login> LoginRequest(RequestsLoader());

    void Login::Initialise()
    {
        Log()->Debug("Initializing Login");
    }

    void Login::Incoming(const Interface::Player::Shared & player, const Message::Shared & message)
    {
        Log()->Debug("Incoming");
        player->Log()->Debug("Message: {}", message->ToString());

        const auto & model = player->Model();
        if (model->GetPlayerType() != PlayerAnonymous)
        {
            return SendFail(player, "already_logged");
        }

        auto & request = message->GetBody();
        if (request.contains("token"))
        {
            /// token auth

            const std::string token = request["token"].as_string().c_str();
        }
        else
        {
            /// log + pass auth

            const std::string login = request["login"].as_string().c_str();
            const std::string password = request["password"].as_string().c_str();

            model->Login(login, password) = [=, this](bool success) {
                if (!success)
                    return SendFail(player, "user_not_found");

                SendResponse(player, {{"success", true}});
            };
        }

        // boost::json::object response;
        //
        // SendResponse(player, response);
    }
}
