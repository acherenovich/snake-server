#include "servers/internal/server.hpp"
#include "[core_loader].hpp"

namespace Core::Servers::Internal {
    [[maybe_unused]] Utils::Service::Loader::Add<InternalServer> ServersInternalService(ServicesLoader());
} // namespace Core::Servers::Internal
