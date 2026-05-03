#pragma once

#include "servers/websocket/interfaces/server.hpp"

namespace Core::Servers::Internal::Interface {
    // Отдельный тип для internal WS сервера — чтобы IFace().Get<> различал его
    // от публичного WebSocket сервера
    class Server : public Websocket::Interface::Server
    {
    public:
        using Shared = std::shared_ptr<Server>;
    };
}
