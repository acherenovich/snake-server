#include "leaderboard.hpp"

/*

{
    "serverId": 123
}

{
    "leaderboard": [
    {
        "name": "aaa",
        "exp": 123,
    }

    ],
}

*/

namespace Core::App::PlayerSession::Requests {

    [[maybe_unused]] Utils::Service::Loader::Add<LeaderBoard> LeaderBoardRequest(RequestsLoader());

    void LeaderBoard::Initialise()
    {
        Log()->Debug("Initializing LeaderBoard");
    }

    void LeaderBoard::OnAllInterfacesLoadedPost()
    {
        gameController_ = IFace().Get<GameController>();
    }

    void LeaderBoard::Incoming(const Interface::Player::Shared & player, const Message::Shared & message)
    {
        const auto sourceJobID = message->GetHeaders()->GetSourceJobID();
        Log()->Debug("Incoming");

        const auto & model = player->Model();
        if (model->GetPlayerType() == PlayerAnonymous)
        {
            return SendFail(player, "not_logged", sourceJobID);
        }

        auto & request = message->GetBody();

        if (!request.contains("serverId") || !request["serverId"].is_int64())
            return SendFail(player, "error", sourceJobID);

        uint32_t serverID = request["serverId"].as_int64();

        const auto servers = gameController_->GetGameServers();
        if (serverID > servers.size())
            return SendFail(player, "error", sourceJobID);

        auto leaderboard = servers[serverID - 1]->GetLeaderboard();

        boost::json::array sessionsJson;
        for (auto & [targetPlayer, exp]: leaderboard)
        {
            boost::json::object row;
            row["name"] = targetPlayer->Model()->GetLogin();
            row["exp"] = exp;
            sessionsJson.push_back(row);
        }

        SendSuccess(player, {{"leaderboard", sessionsJson}}, sourceJobID);
    }
}
