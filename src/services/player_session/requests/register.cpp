#include "register.hpp"

/*

{
    "login": "login",
    "password": "password"
}

{
    "success": true,
    "token": "ABABABABBABABBA",
    "message": "user_not_found"
}

*/

namespace Core::App::PlayerSession::Requests {

    [[maybe_unused]] Utils::Service::Loader::Add<Register> RegisterRequest(RequestsLoader());

    void Register::Initialise()
    {
        Log()->Debug("Initializing Register");
    }

    void Register::Incoming(const Interface::Player::Shared & player, const Message::Shared & message)
    {
        const auto sourceJobID = message->GetHeaders()->GetSourceJobID();
        Log()->Debug("Incoming");

        const auto & model = player->Model();
        if (model->GetPlayerType() != PlayerAnonymous)
        {
            return SendFail(player, "already_logged", sourceJobID);
        }

        auto & request = message->GetBody();

        /// log + pass register
        const std::string login = NormalizeLogin(request["login"].as_string().c_str()).value_or("");
        const std::string password = NormalizePassword(request["password"].as_string().c_str()).value_or("");

        if (login.empty())
            return SendFail(player, "malformed_login", sourceJobID);
        if (password.empty())
            return SendFail(player, "malformed_password", sourceJobID);

        model->Register(login, password) = [=, this](bool success) {
            if (!success)
                return SendFail(player, "failed_to_register", sourceJobID);

            SendSuccess(player, {{"player", player->Serialise()}}, sourceJobID);
        };


        // boost::json::object response;
        //
        // SendSuccess(player, response);
    }
}
