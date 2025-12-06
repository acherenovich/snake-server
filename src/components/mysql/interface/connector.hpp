#pragma once

#include "[core_loader].hpp"
#include "coroutine.hpp"

#include <boost/json.hpp>
#include <array>
#include <string>
#include <type_traits>

namespace Core::Components::MySQL {
    namespace Interface {

        class RequestResult
        {
        public:
            virtual ~RequestResult() = default;

            using Shared = std::shared_ptr<RequestResult>;

            [[nodiscard]] virtual bool IsSuccess() const = 0;

            virtual boost::json::array & Get() = 0;

            virtual boost::json::object & Get(size_t n) = 0;

            [[nodiscard]] virtual size_t Count() const = 0;
        };

        class Connector : public BaseServiceInterface
        {
        public:
            using Shared = std::shared_ptr<Connector>;

            ~Connector() override = default;

            // Базовый "сыро-строковый" запрос — реализуется в имплементации
            virtual Utils::Task<RequestResult::Shared> Query(const std::string & query) = 0;

        protected:
            // Реализация escape — делает конкретный MySQL-клиент
            virtual std::string EscapeString(const std::string & s) = 0;

            // Универсальный SQL literal
            template<typename T>
            std::string ToSqlLiteral(const T & value)
            {
                using Decayed = std::decay_t<T>;

                if constexpr (std::is_same_v<Decayed, std::nullptr_t>)
                {
                    return "NULL";
                }
                else if constexpr (std::is_same_v<Decayed, bool>)
                {
                    return value ? "1" : "0";
                }
                else if constexpr (std::is_integral_v<Decayed> || std::is_floating_point_v<Decayed>)
                {
                    return std::to_string(value);
                }
                else if constexpr (std::is_convertible_v<Decayed, std::string_view>)
                {
                    std::string s { std::string_view(value) };
                    return "'" + EscapeString(s) + "'";
                }
                else
                {
                    static_assert(!sizeof(T), "Unsupported SQL literal type");
                }
                return "NULL";
            }

            template<typename... Args>
            std::string BuildSqlFromFormat(std::string_view fmt, const Args &... args)
            {
                // Собираем все параметры сразу
                std::array<std::string, sizeof...(Args)> params {
                    ToSqlLiteral(args)...
                };

                std::string result;
                result.reserve(fmt.size() + params.size() * 8);

                std::size_t argIndex = 0;

                for (std::size_t i = 0; i < fmt.size(); ++i)
                {
                    char c = fmt[i];

                    if (c == '{')
                    {
                        if (i + 1 < fmt.size() && fmt[i + 1] == '}')
                        {
                            if (argIndex >= params.size())
                            {
                                throw std::runtime_error("Too few arguments for SQL format string");
                            }

                            result += params[argIndex++];
                            ++i; // пропускаем '}'
                            continue;
                        }
                    }

                    result.push_back(c);
                }

                if (argIndex != params.size())
                {
                    throw std::runtime_error("Too many arguments for SQL format string");
                }

                return result;
            }

        public:
            template<typename... Args>
            Utils::Task<RequestResult::Shared>
            Query(std::string_view fmt, const Args &... args)
            {
                const std::string sql = BuildSqlFromFormat(fmt, args...);
                co_return co_await Query(sql);
            }
        };

    } // namespace Interface
} // namespace Core::Components::MySQL