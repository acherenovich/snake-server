#include "controller.hpp"
#include "requests/[requests_loader].hpp"

namespace Core::App::PlayerSession
{
    void Controller::Initialise()
    {
        Log()->Debug("Initialised");
    }

    void Controller::OnAllInterfacesLoaded()
    {
        server_ = IFace().Get<Server>();

        server_->RegisterClientsCallback([this](const Client::Shared & client, const Client::Events & event) {
            if (event == Client::Events::ClientConnected)
                return OnClientConnected(client);

            if (event == Client::Events::ClientDisconnected)
                return OnClientDisconnected(client);
        });
    }

    void Controller::CreateSession(uint32_t ownerAccountID)
    {
    }

    Interface::Player::Shared Controller::GetPlayer(const Servers::Websocket::Interface::Client::Shared & client)
    {
        const auto it = players_.find(client);
        if (it == players_.end())
        {
            Log()->Error("GetPlayer: No such player");
            return {};
        }

        return it->second;
    }

    void Controller::OnClientConnected(const Client::Shared & client)
    {
        auto player = Player::Create(this, client);
        players_[client] = player;
    }

    void Controller::OnClientDisconnected(const Client::Shared & client)
    {
        const auto it = players_.find(client);
        if (it == players_.end())
        {
            Log()->Error("OnClientDisconnected: No such player");
            return;
        }

        it->second->Close();

        players_.erase(it);

    }
}