#include "PhysXManager.h"

std::shared_ptr<PhysXManager> PhysXManager::s_instance;

PhysXManager::~PhysXManager()
{
}

std::shared_ptr<PhysXManager> PhysXManager::GetInstance()
{
    if (!s_instance.get()) {
        std::shared_ptr<PhysXManager> newInstance(new PhysXManager());
        s_instance = newInstance;
    }

    return s_instance;
}
