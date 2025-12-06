#pragma once

#include <utility>

#include "interfaces/player.hpp"
#include "models/player.hpp"

#include "servers/websocket/interfaces/server.hpp"

namespace Core::App::PlayerSession
{
    class Player final : public Interface::Player, public std::enable_shared_from_this<Player>
    {
        Servers::Websocket::Interface::Client::Shared client_;
        Model::Player::Shared model_;
    public:
        using Shared = std::shared_ptr<Player>;

        explicit Player(Servers::Websocket::Interface::Client::Shared client): client_(std::move(client)) {};

        void Close();

        Model::Interface::Player::Shared Model() override;

        [[nodiscard]] const Servers::Websocket::Interface::Client::Shared & GetClient() const override;


        void OnSetupContainerSuccess() override
        {
            model_ = Model::Player::Create(this);
        }

        std::string GetServiceContainerName() const override
        {
            return "PLAYER";
        }

        static Shared Create(const BaseServiceContainer * parent, const Servers::Websocket::Interface::Client::Shared & client)
        {
            const auto player = std::make_shared<Player>(client);
            player->SetupContainer(parent);
            return player;
        }
    };

}