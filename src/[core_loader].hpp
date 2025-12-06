#pragma once

#include <logging.hpp>
#include <service_loader.hpp>
#include <interface_controller.hpp>

namespace Core {
    using Loader = Utils::Service::Loader;
    using InterfaceController = Utils::Service::InterfaceController;

    inline Loader & ServicesLoader()
    {
        static Loader loader;
        return loader;
    }

    inline InterfaceController & GetInterfaceController()
    {
        static InterfaceController interfaceController;
        return interfaceController;
    }

    using BaseServiceInterface = Utils::Service::BaseServiceInterface;

    class BaseServiceContainer: public Utils::Service::BaseServiceContainerTemplate
    {
        Utils::Logging::Logger::Shared logger_ {};

    public:
        using Shared = std::shared_ptr<BaseServiceContainer>;

        explicit BaseServiceContainer(const Utils::Logging::Logger::Shared & logger): logger_(logger) {}
        explicit BaseServiceContainer(): logger_(Utils::Log()) {}

        ~BaseServiceContainer() override = default;

        virtual const Utils::Logging::Logger::Shared & Log() const
        {
            return logger_;
        }

        void SetupContainer(const BaseServiceContainer & parent)
        {
            logger_ = parent.logger_->CreateChild(GetServiceContainerName());
        }

        void SetupContainer(const BaseServiceContainer * parent)
        {
            SetupContainer(*parent);
        }

        virtual InterfaceController & IFace() const
        {
            return GetInterfaceController();
        }
    };

    class BaseServiceInstance : public Utils::Service::Instance, public BaseServiceContainer {
    public:
        using Shared = std::shared_ptr<BaseServiceInstance>;

        explicit BaseServiceInstance() {}

        ~BaseServiceInstance() override = default;
    };

} // namespace Core