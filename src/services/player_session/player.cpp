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
}