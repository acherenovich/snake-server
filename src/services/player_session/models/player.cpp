#include "player.hpp"

#include "crypt.hpp"
#include "utils.hpp"

namespace Core::App::PlayerSession::Model
{
    PlayerType Player::GetPlayerType() const
    {
        return type_;
    }

    Utils::Task<bool> Player::Register(const std::string login, const std::string password)
    {
        login_ = login;
        hashedPassword_ = Utils::Crypt::GetSHA256(password);

        const auto found = co_await Database()->Query("select id from `snake_players` where login = {}", login_);

        if (!found->IsSuccess() || found->Count())
        {
            Log()->Debug("(Register failed) Player with login '{}' already exists", login_);
            co_return false;
        }

        const auto insert = co_await Database()->Query("insert into `snake_players` (`login`, `password`) VALUES ({}, {});", login_, hashedPassword_);
        if (!insert->IsSuccess())
        {
            Log()->Debug("(Register failed) Insert login '{}' pass '{}' failed", login_, hashedPassword_);
            co_return false;
        }

        userID_ = insert->InsertID();

        type_ = PlayerBase;

        PrepareToken();

        co_return true;
    }

    Utils::Task<bool> Player::Login(const std::string login, const std::string password)
    {
        login_ = login;
        hashedPassword_ = Utils::Crypt::GetSHA256(password);

        const auto found = co_await Database()->Query("select id, experience from `snake_players` where login = {} and password = {}", login_, hashedPassword_);

        if (!found->IsSuccess() || !found->Count())
        {
            co_return false;
        }

        userID_ = std::stoul(found->Get(0)["id"].as_string().c_str());
        userExp_ = std::stoul(found->Get(0)["experience"].as_string().c_str());

        type_ = PlayerBase;

        PrepareToken();

        co_return true;
    }

    Utils::Task<bool> Player::Login(std::string token)
    {
        boost::json::object object;
        std::string error;

        auto success = Utils::Crypt::ValidateJwtHs256(token, Utils::Env("SESSION_SECRET"), object, error, true);
        if (!success)
        {
            Log()->Error("(Login failed) Invalid token '{}' ({})", token, error);
            co_return false;
        }

        try
        {
            boost::json::object data = object["data"].as_object();
            userID_ = data["id"].as_int64();
            login_ = data["login"].as_string();
            hashedPassword_ = data["password"].as_string();
            token_ = token;
        }
        catch (const std::exception& e)
        {
            Log()->Error("(Login failed) Invalid token '{}' ({})", token, e.what());
            co_return false;
        }

        const auto found = co_await Database()->Query("select experience from `snake_players` where id = {}", userID_);

        if (!found->IsSuccess() || !found->Count())
        {
            co_return false;
        }

        userExp_ = std::stoul(found->Get(0)["experience"].as_string().c_str());

        type_ = PlayerBase;

        PrepareToken();

        co_return true;
    }

    [[nodiscard]] boost::json::object Player::Serialise(const SerialiseType & serialiseType)
    {
        boost::json::object result;

        result["id"] = userID_;
        result["login"] = login_;

        if (serialiseType == SerialisePlayer)
        {
            result["experience"] = userExp_;
            result["playerType"] = PlayerTypeMap[type_];
            result["token"] = token_;
        }

        if (serialiseType == SerialiseToken)
        {
            result["password"] = hashedPassword_;
        }

        return result;
    }

    void Player::PrepareToken()
    {
        if (type_ == PlayerAnonymous)
        {
            Log()->Error("Cant create token for anonymous user");
            return;
        }

        const auto object = Serialise(SerialiseToken);

        token_ = Utils::Crypt::GenerateJwtHs256(object, Utils::Env("SESSION_SECRET"), "player_session", "client", std::chrono::days(7));
    }
}
