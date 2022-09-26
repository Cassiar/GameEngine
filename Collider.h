#pragma once
#include "Transform.h"
#include "Mesh.h"

#include <memory>

class Collider
{
private:
	DirectX::XMFLOAT3 m_maxPoint;
	DirectX::XMFLOAT3 m_minPoint;
	DirectX::XMFLOAT3 m_centerPoint;

	float m_halfWidth;
	float m_halfHeight;
	float m_halfDepth;

	float m_preCheckRadiusSquared;

	std::shared_ptr<Mesh> m_objectMesh;

	bool m_pointsDirty;
	bool m_halvesDirty;

	void CalcMinMaxPoints();
	void CalcHalfDimensions();
	void CalcCenterPoint();

public:
	Collider(std::shared_ptr<Mesh> m_colliderMesh);
	~Collider();

	std::shared_ptr<Mesh> GetCollisionMesh() { return m_objectMesh; }

	bool CheckForCollision(const std::shared_ptr<Collider> other);
	void MakePointsDirty() { m_pointsDirty = true; }
	void MakeHalvesDirty() { m_pointsDirty = true; }
};

