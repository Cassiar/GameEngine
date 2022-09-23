#include "EntityManager.h"

EntityManager::EntityManager()
{
    l_entities = std::vector<std::shared_ptr<GameEntity>>();
}

//Not needed atm
EntityManager::~EntityManager()
{
}

std::shared_ptr<EntityManager> EntityManager::GetInstance()
{
    if (!m_instance.get()) {
        m_instance = std::make_shared<EntityManager>();
    }

    return m_instance;
}

bool EntityManager::SetEntity(const int index, std::shared_ptr<GameEntity> entity)
{
    if (IndexInBounds(index)) {
        return false;
    }

    l_entities[index] = entity;
    return true;
}

bool EntityManager::InsertEntity(const int index, std::shared_ptr<GameEntity> entity)
{
    if (IndexInBounds(index)) {
        return false;
    }

    l_entities.insert(l_entities.begin() + index, entity);
    return true;
}

void EntityManager::AddEntity(std::shared_ptr<GameEntity> entity)
{
    l_entities.push_back(entity);
}
