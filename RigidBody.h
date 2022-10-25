#pragma once
#include "Transform.h"

#include <memory>

class RigidBody
{
private:
	const float GRAVITY = 10.0f;

	DirectX::XMFLOAT3 m_velocity;
	DirectX::XMFLOAT3 m_acceleration;

	Transform* m_transform;

	bool m_hasGravity;

public:
	RigidBody(Transform* parentTransform);
	~RigidBody();

	void ToggleGravity() { m_hasGravity = !m_hasGravity; }
	void UpdateTransform(float dt);
};

