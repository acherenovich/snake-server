#include "connect_udp.hpp"

/*

{
    "ssid": 123,
    "serverId": 123
}

{
}

*/

namespace Core::App::PlayerSession::Requests {

    [[maybe_unused]] Utils::Service::Loader::Add<ConnectUDP> ConnectUDPRequest(RequestsLoader());

    void ConnectUDP::Initialise()
    {
        Log()->Debug("Initializing ConnectUDP");
    }

    void ConnectUDP::OnAllInterfacesLoadedPost()
    {
        gameController_ = IFace().Get<GameController>();
    }

    void ConnectUDP::Incoming(const Interface::Player::Shared & player, const Message::Shared & message)
    {
        const auto sourceJobID = message->GetHeaders()->GetSourceJobID();
        Log()->Debug("Incoming");

        const auto & model = player->Model();
        if (model->GetPlayerType() == PlayerAnonymous)
        {
            return SendFail(player, "not_logged", sourceJobID);
        }

        auto & request = message->GetBody();

        if (!request.contains("ssid") || !request["ssid"].is_int64())
            return SendFail(player, "error", sourceJobID);

        if (!request.contains("serverId") || !request["serverId"].is_int64())
            return SendFail(player, "error", sourceJobID);

        uint64_t ssid = request["ssid"].as_int64();
        uint32_t serverID = request["serverId"].as_int64();

        const auto servers = gameController_->GetGameServers();
        if (serverID > servers.size())
            return SendFail(player, "error", sourceJobID);

        servers[serverID - 1]->SetSSIDPlayer(ssid, player);

        SendSuccess(player, {}, sourceJobID);
    }
}
