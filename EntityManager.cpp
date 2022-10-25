#include "EntityManager.h"

std::shared_ptr<EntityManager> EntityManager::s_instance;

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
    if (!s_instance.get()) {
        std::shared_ptr<EntityManager> newInstance(new EntityManager());
        s_instance = newInstance;
    }

    return s_instance;
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

void const EntityManager::DrawEntities(bool prepareMat)
{
    //loop and draw all objects in range of shadow
    // '&' is important because it prevents making copies
    for (auto& entity : l_entities) {
        if (prepareMat) {
            entity->GetMaterial()->PrepareMaterial();
        }

        //draw. Don't need material
        entity->GetMesh()->Draw();
    }
}

void EntityManager::UpdateEntities(float dt)
{
    for (auto& entity : l_entities)
    {
        entity->Update(dt, l_entities);
    }
}
