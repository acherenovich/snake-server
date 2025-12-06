#include "connector.hpp"

#include "utils.hpp"

namespace Core::Components::MySQL {
    void Connector::Initialise()
    {
        Config config;
        config.host   = Utils::Env("MYSQL_HOST");
        config.port   = Utils::EnvInt("MYSQL_PORT", 3306);

        config.user   = Utils::Env("MYSQL_USER");
        config.password   = Utils::Env("MYSQL_PASSWORD");
        config.database   = Utils::Env("MYSQL_DATABASE");

        config.poolSize   = Utils::EnvInt("MYSQL_THREADS", 1);

        client_ = Client::Create(config, Log());

        Log()->Debug("MySQL connected");
    }

    void Connector::OnAllServicesLoaded()
    {
        Log()->Debug("OnAllServicesLoaded()");

        IFace().Register<Interface::Connector>(shared_from_this());
    }

    void Connector::OnAllInterfacesLoaded()
    {
        Log()->Debug("OnAllInterfacesLoaded()");
    }

    void Connector::ProcessTick()
    {
        client_->ProcessTick();
    }

    Utils::Task<Interface::RequestResult::Shared> Connector::Query(const std::string & query)
    {
        auto result = co_await client_->Execute(query);

        co_return std::make_shared<RequestResult>(result);
    }

    std::string Connector::EscapeString(const std::string& s)
    {
        return client_->EscapeString(s);
    }


}
