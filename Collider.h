#pragma once
#include "Transform.h"
#include "Mesh.h"

#include <memory>

class Collider
{
private:
	DirectX::XMFLOAT3 m_maxPoint;
	DirectX::XMFLOAT3 m_minPoint;

	std::shared_ptr<Mesh> m_colliderMesh;

	float m_preCheckRadius;

	float FindPreCheckRadius();
	void CalcAABB();

public:
	Collider(std::shared_ptr<Mesh> m_colliderMesh);
	~Collider();

	std::shared_ptr<Mesh> GetCollisionMesh() { return m_colliderMesh; }

	bool CheckForCollision(const std::shared_ptr<Collider> other);
};

