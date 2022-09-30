#pragma once
#include "Transform.h"
#include "Mesh.h"
#include "Camera.h"

#include <memory>

class Collider
{
private:
	std::shared_ptr<Mesh> m_objectMesh;
	std::vector<DirectX::XMFLOAT4> l_transformedPositions;
	std::vector<DirectX::XMFLOAT3> l_transformedCubeVerts;
	Transform* m_transform;

	DirectX::XMFLOAT3 m_maxPoint;
	DirectX::XMFLOAT3 m_minPoint;
	DirectX::XMFLOAT3 m_centerPoint;

	float m_halfWidth;
	float m_halfHeight;
	float m_halfDepth;

	float m_preCheckRadiusSquared;

	bool m_pointsDirty;
	bool m_halvesDirty;

	void CalcMinMaxPoints();
	void CalcHalfDimensions();
	void CalcCenterPoint();

	bool CheckGJKCollision(const std::shared_ptr<Collider> other);
	DirectX::XMVECTOR CalcSupport(const DirectX::XMVECTOR& direction);
	bool DoSimplex(std::vector<DirectX::XMVECTOR>& supports, DirectX::XMVECTOR& direction);

	Transform* m_sphere;
	std::shared_ptr<Camera> m_camera;

public:
	Collider(std::shared_ptr<Mesh> colliderMesh, Transform* transform);
	Collider(std::shared_ptr<Mesh> colliderMesh, Transform* transform, Transform* sphere, std::shared_ptr<Camera> cam);
	~Collider();

	std::shared_ptr<Mesh> GetCollisionMesh() { return m_objectMesh; }

	bool CheckForCollision(const std::shared_ptr<Collider> other);
	void MakePointsDirty() { m_pointsDirty = true; }
	void MakeHalvesDirty() { m_pointsDirty = true; }
};

