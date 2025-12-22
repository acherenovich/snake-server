#pragma once

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>

#include "double_map.hpp"

#include "[core_loader].hpp"

namespace Core::App::PlayerSession
{
    enum PlayerType
    {
        PlayerAnonymous,
        PlayerBase,
    };

    static DoubleMap<PlayerType> PlayerTypeMap {
        {
            {PlayerAnonymous, "anonymous"},
            {PlayerBase, "base"},
        }
    };

    [[nodiscard]] inline std::optional<std::string> NormalizeLogin(const std::string & input)
    {
        std::string login = input;

        // to lower (ASCII only)
        std::ranges::transform(login, login.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        static const std::regex loginRegex{"^[a-z0-9]+$"};

        if (!std::regex_match(login, loginRegex))
            return std::nullopt;

        return login;
    }

    [[nodiscard]] inline std::optional<std::string> NormalizePassword(const std::string & input)
    {
        constexpr std::string_view allowedSymbols = "!@#$%^&*()_+-=";

        if (input.size() < 6 || input.size() > 64)
            return std::nullopt;

        std::string result;
        result.reserve(input.size());

        for (unsigned char c : input)
        {
            if (std::isspace(c))
                return std::nullopt;

            if (std::isalnum(c) || allowedSymbols.find(c) != std::string_view::npos)
            {
                result.push_back(static_cast<char>(c));
            }
            else
            {
                return std::nullopt;
            }
        }

        return result;
    }
}