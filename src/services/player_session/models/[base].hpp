#pragma once

#include "../interfaces/models/[base].hpp"
#include "components/mysql/interface/connector.hpp"

namespace Core::App::PlayerSession::Model
{
    class BaseModel: public virtual BaseServiceContainer
    {
        Components::MySQL::Interface::Connector::Shared connector_;
    public:
        using Shared = std::shared_ptr<BaseModel>;

        BaseModel()
        {
            connector_ = BaseServiceContainer::IFace().Get<Components::MySQL::Interface::Connector>();
        }

        [[nodiscard]] const Components::MySQL::Interface::Connector::Shared & Database() const
        {
            return connector_;
        }
    };
}