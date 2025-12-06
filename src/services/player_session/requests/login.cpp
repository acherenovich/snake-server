#include "login.hpp"

namespace Core::App::PlayerSession::Requests {

    [[maybe_unused]] Utils::Service::Loader::Add<Login> LoginRequest(RequestsLoader());

    void Login::Initialise()
    {
        Log()->Debug("Initializing Login");
    }

    void Login::Incoming(const Client::Shared & client, const Message::Shared & message)
    {
        Log()->Debug("Incoming");
        client->Log()->Debug("Message: {}", message->ToString());
    }
}
