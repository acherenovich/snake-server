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
        const auto sourceJobID = message->GetHeaders()->GetSourceJobID();
        Log()->Debug("Incoming");

        const auto & model = player->Model();
        if (model->GetPlayerType() != PlayerAnonymous)
        {
            return SendFail(player, "already_logged", sourceJobID);
        }

        auto & request = message->GetBody();
        if (request.contains("token"))
        {
            /// token auth

            const std::string token = request["token"].as_string().c_str();

            model->Login(token) = [=, this](bool success) {
                if (!success)
                    return SendFail(player, "token_out_of_date", sourceJobID);

                SendSuccess(player, {{"player", player->Serialise()}}, sourceJobID);
            };
        }
        else
        {
            /// log + pass auth

            const std::string login = NormalizeLogin(request["login"].as_string().c_str()).value_or("");
            const std::string password = NormalizePassword(request["password"].as_string().c_str()).value_or("");

            if (login.empty())
                return SendFail(player, "malformed_login", sourceJobID);
            if (password.empty())
                return SendFail(player, "malformed_password", sourceJobID);

            model->Login(login, password) = [=, this](bool success) {
                if (!success)
                    return SendFail(player, "user_not_found", sourceJobID);

                SendSuccess(player, {{"player", player->Serialise()}}, sourceJobID);
            };
        }

        // boost::json::object response;
        //
        // SendSuccess(player, response);
    }
}
