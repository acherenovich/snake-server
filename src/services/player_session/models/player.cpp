#include "player.hpp"

#include "crypt.hpp"

namespace Core::App::PlayerSession::Model
{
    PlayerType Player::GetPlayerType() const
    {
        return type_;
    }


    Utils::Task<bool> Player::Login(const std::string login, const std::string password)
    {
        const auto found = co_await Database()->Query("select id from `snake_players` where login = {} and password = {}", login, Utils::Crypt::GetSHA256(password));

        co_return found->IsSuccess() && found->Count();
    }

    Utils::Task<bool> Player::Login(std::string token)
    {
        co_return false;
    }
}