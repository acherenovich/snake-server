#pragma once

#include "interfaces/controller.hpp"

#include "requests/[requests_loader].hpp"

namespace Core::App::PlayerSession
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;

        void CreateSession(uint32_t ownerAccountID) override;

        std::string GetServiceContainerName() const override
        {
            return "PL_SES";
        }

        std::vector<Utils::Service::SubLoader> SubLoaders() override
        {
            static Requests::RequestsServiceContainer container{shared_from_this()};

            return
            {
                {
                    Requests::RequestsLoader(),
                    [](auto child_) {
                        const auto child = dynamic_cast<Requests::RequestsServiceContainer*>(child_);
                        child->SetupContainer(container);
                    }
                }
            };
        };
    };

}