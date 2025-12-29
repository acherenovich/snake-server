#include "stats.hpp"

/*

{
}

{
    "sessions": [
        {
            "id": 1,
            "players": 1
        }
    ]
}

*/

namespace Core::App::PlayerSession::Requests {

    [[maybe_unused]] Utils::Service::Loader::Add<Stats> StatsRequest(RequestsLoader());

    void Stats::Initialise()
    {
        Log()->Debug("Initializing Stats");
    }

    void Stats::OnAllInterfacesLoadedPost()
    {
        gameController_ = IFace().Get<GameController>();
    }

    void Stats::Incoming(const Interface::Player::Shared & player, const Message::Shared & message)
    {
        const auto sourceJobID = message->GetHeaders()->GetSourceJobID();
        Log()->Debug("Incoming");

        const auto & model = player->Model();
        if (model->GetPlayerType() == PlayerAnonymous)
        {
            return SendFail(player, "not_logged", sourceJobID);
        }

        boost::json::array sessionsJson;
        for (const auto& gameServer: gameController_->GetGameServers())
        {
            boost::json::object session;
            session["id"] = gameServer->GetServerID();
            session["players"] = gameServer->GetPlayersCount();
            sessionsJson.push_back(session);
        }

        SendSuccess(player, {{"sessions", sessionsJson}}, sourceJobID);
    }
}
