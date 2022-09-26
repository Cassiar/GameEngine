#include "RigidBody.h"

using namespace DirectX;

RigidBody::RigidBody(Transform* parentTransform)
	: m_transform(parentTransform),
	m_velocity(XMFLOAT3(0.0f, 0.0f, 0.0f)),
	m_acceleration(XMFLOAT3(0.0f, 0.0f, 0.0f))
{
}

RigidBody::~RigidBody()
{
}

void RigidBody::UpdateTransform(float dt) 
{
	XMVECTOR velocity = XMLoadFloat3(&m_velocity);
	velocity += dt * XMLoadFloat3(&m_acceleration);

	XMStoreFloat3(&m_velocity, velocity);

	if (m_hasGravity)
	{
		m_velocity.y += GRAVITY * dt;
	}

	m_transform->MoveRelative(dt * m_velocity.x, dt * m_velocity.y, dt * m_velocity.z);
}