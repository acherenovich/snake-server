#include "connector.hpp"
#include "[core_loader].hpp"

namespace Core::Components::MySQL {
    [[maybe_unused]] Utils::Service::Loader::Add<Connector>DBMySQL(ServicesLoader());
} // namespace Core::Servers::Websocket