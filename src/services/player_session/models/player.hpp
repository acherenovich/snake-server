#pragma once

#include <utility>

#include "[base].hpp"
#include "../interfaces/models/player.hpp"

namespace Core::App::PlayerSession::Model
{
    class Player final :
        public Interface::Player,
        public BaseModel,
        public std::enable_shared_from_this<Player>
    {
        PlayerType type_ = PlayerAnonymous;
    public:
        using Shared = std::shared_ptr<Player>;

        explicit Player() {};

        PlayerType GetPlayerType() const override;

        Utils::Task<bool> Login(std::string login, std::string password) override;

        Utils::Task<bool> Login(std::string token) override;

        std::string GetServiceContainerName() const override
        {
            return "MDL";
        }

        static Shared Create(const BaseServiceContainer * parent)
        {
            const auto player = std::make_shared<Player>();
            player->SetupContainer(parent);
            return player;
        }
    };

}