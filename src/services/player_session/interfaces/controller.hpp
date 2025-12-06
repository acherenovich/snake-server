#pragma once

#include "[core_loader].hpp"

namespace Core::App::PlayerSession
{
    namespace Interface
    {
        class Controller: public virtual BaseServiceInstance
        {
        public:
            using Shared = std::shared_ptr<Controller>;

            virtual void CreateSession(uint32_t ownerAccountID) = 0;

        };
    }
}