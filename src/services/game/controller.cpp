#include "controller.hpp"

namespace Core::App::Game
{
    void Controller::Initialise()
    {
        for (const auto serverID: {1, 2, 3})
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

    std::vector<Interface::GameServer::Shared> Controller::GetGameServers() const
    {
        std::vector<Interface::GameServer::Shared> gameServers;
        gameServers.reserve(gameServers_.size());
        for (const auto& gameServer_: gameServers_)
            gameServers.emplace_back(gameServer_);

        return gameServers;
    }

}
