#pragma once

#include "interface/connector.hpp"

#include "mysql.hpp"

namespace Core::Components::MySQL {
    using namespace Utils::DB::MySQL;

    class RequestResult final : public Interface::RequestResult
    {
        QueryResult queryResult_;
    public:
        using Shared = std::shared_ptr<RequestResult>;

        explicit RequestResult(QueryResult & queryResult): queryResult_(std::move(queryResult)) {}

        [[nodiscard]] bool IsSuccess() const override
        {
            return queryResult_.success;
        }

        boost::json::array & Get() override
        {
            return queryResult_.rows;
        }

        boost::json::object & Get(const size_t n) override
        {
            return queryResult_.rows[n].as_object();
        }

        [[nodiscard]] size_t Count() const override
        {
            return queryResult_.rows.size();
        }

        [[nodiscard]] uint64_t InsertID() const override
        {
            return queryResult_.insertId;
        }
    };

    class Connector final:
        public Interface::Connector,
        public BaseServiceInstance,
        public std::enable_shared_from_this<Connector>
    {
        Utils::DB::MySQL::Client::Shared client_;

    public:
        using Shared    = std::shared_ptr<Connector>;

        Connector() = default;
        ~Connector() override = default;

        Utils::Task<Interface::RequestResult::Shared> Query(const std::string & query) override;

        std::string EscapeString(const std::string& s) override;
    protected:
        void Initialise() override;
        void OnAllServicesLoaded() override;
        void OnAllInterfacesLoaded() override;
        void ProcessTick() override;

    private:
        std::string GetServiceContainerName() const override
        {
            return "MySQL";
        }
    };
}