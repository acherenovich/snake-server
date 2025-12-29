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

        uint32_t userID_ = 0;
        uint32_t userExp_ = 0;

        std::string login_, hashedPassword_;
        std::string token_;
    public:
        using Shared = std::shared_ptr<Player>;

        explicit Player() {};

        PlayerType GetPlayerType() const override;

        Utils::Task<bool> Register(std::string login, std::string password) override;

        Utils::Task<bool> Login(std::string login, std::string password) override;

        Utils::Task<bool> Login(std::string token) override;

        std::string GetLogin() const override;


        [[nodiscard]] boost::json::object Serialise(const SerialiseType & serialiseType = SerialisePlayer) override;

    private:
        void PrepareToken();

        std::string GetServiceContainerName() const override
        {
            return "MDL";
        }

    public:
        static Shared Create(const BaseServiceContainer * parent)
        {
            const auto player = std::make_shared<Player>();
            player->SetupContainer(parent);
            return player;
        }
    };

}