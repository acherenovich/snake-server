#include "servers/websocket/server.hpp"
#include "[core_loader].hpp"

namespace Core::Servers::Websocket {
    [[maybe_unused]] Utils::Service::Loader::Add<WebsocketService>ServersWebsocketService(ServicesLoader());
} // namespace Core::Servers::Websocket