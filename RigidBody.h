#pragma once
#include "Transform.h"
#include "Collider.h"

#include <memory>

class RigidBody
{
private:
	const float GRAVITY = 10.0f;

	DirectX::XMFLOAT3 m_velocity;
	DirectX::XMFLOAT3 m_acceleration;

	std::shared_ptr<Transform> m_transform;
	std::shared_ptr<Mesh> m_collisionMesh;

	bool m_hasGravity;
	float m_preCheckRadius;

	float FindPreCheckRadius();
	void CalcAABB();

public:
	RigidBody(Transform* parentTransform);
	~RigidBody();

	std::shared_ptr<Mesh> GetCollisionMesh() { return m_collisionMesh; }

	void ToggleGravity() { m_hasGravity = !m_hasGravity; }
	void UpdateTransform(float dt);

	bool CheckForCollision(std::shared_ptr<RigidBody> other);
};

