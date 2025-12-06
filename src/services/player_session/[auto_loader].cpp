#include "controller.hpp"

namespace Core::App::PlayerSession {
    [[maybe_unused]] Utils::Service::Loader::Add<Controller> AppPlayerSession(ServicesLoader());
} // namespace Core::Servers::Websocket