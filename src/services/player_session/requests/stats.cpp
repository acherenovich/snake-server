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

        SendStats(player, sourceJobID) = [this, player, sourceJobID](const bool success) {
            if (!success)
                SendFail(player, "error", sourceJobID);
        };
    }

    Utils::Task<bool> Stats::SendStats(const Interface::Player::Shared player, const uint64_t sourceJobID)
    {
        boost::json::array sessionsJson;
        for (const auto& [serverID, gameServer]: gameController_->GetGameServers())
        {
            boost::json::object session;
            session["id"]      = serverID;
            session["players"] = gameServer->GetPlayersCount();
            session["host"]    = gameServer->GetHost();
            session["port"]    = gameServer->GetPort();
            sessionsJson.push_back(session);
        }

        const auto experience = co_await player->Model()->RefreshExperience();

        SendSuccess(player, {
            {"sessions", sessionsJson},
            {"experience", static_cast<int64_t>(experience)}
        }, sourceJobID);

        co_return true;
    }
}
