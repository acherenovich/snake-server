#include "controller.hpp"

namespace Core::App::Game
{
    void Controller::Initialise()
    {
        for (const auto serverID: {1, 2, 3, 4})
            gameServers_.push_back(GameServer::Create(this, serverID));
    }

    void Controller::OnAllServicesLoaded()
    {
        IFace().Register<Interface::Controller>(shared_from_this());
    }

    void Controller::OnAllInterfacesLoaded()
    {
        websocket_ = IFace().Get<Server>();
    }

    void Controller::ProcessTick()
    {
        for (const auto& gameServer_: gameServers_)
            gameServer_->ProcessTick();
    }
}
