#pragma once
#include "GameEntity.h"
#include <vector>


class EntityManager
{
private:
	static std::shared_ptr<EntityManager> m_instance;
	std::vector<std::shared_ptr<GameEntity>> l_entities;

	bool IndexInBounds(const int index) { return index >= 0 && index < l_entities.size(); }

	EntityManager();
	~EntityManager();

public:
	static std::shared_ptr<EntityManager> GetInstance();

	std::shared_ptr<GameEntity> GetEntity(const int index) { return l_entities[index]; }
	bool SetEntity(const int index, std::shared_ptr<GameEntity> entity);
	bool InsertEntity(const int index, std::shared_ptr<GameEntity> entity);
	void AddEntity(std::shared_ptr<GameEntity> entity);

	void SetEntities(std::vector<std::shared_ptr<GameEntity>> entities) { l_entities = entities; }
};

