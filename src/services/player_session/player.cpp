#include "player.hpp"

namespace Core::App::PlayerSession
{
    void Player::Close()
    {

    }

    Model::Interface::Player::Shared Player::Model()
    {
        return model_;
    }

    [[nodiscard]] const Servers::Websocket::Interface::Client::Shared & Player::GetClient() const
    {
        return client_;
    }

    [[nodiscard]] boost::json::object Player::Serialise()
    {
        boost::json::object result;

        result["model"] = model_->Serialise();

        return result;
    }
}