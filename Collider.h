#pragma once
#include "Transform.h"
#include "Mesh.h"
#include "Camera.h"

#include <memory>

enum eSATResults
{
	SAT_NONE = 0,

	SAT_AX,
	SAT_AY,
	SAT_AZ,

	SAT_BX,
	SAT_BY,
	SAT_BZ,

	SAT_AXxBX,
	SAT_AXxBY,
	SAT_AXxBZ,

	SAT_AYxBX,
	SAT_AYxBY,
	SAT_AYxBZ,

	SAT_AZxBX,
	SAT_AZxBY,
	SAT_AZxBZ,
};

class Collider
{
private:
	std::shared_ptr<Mesh> m_objectMesh;
	std::vector<DirectX::XMFLOAT4> l_transformedPositions;
	std::vector<DirectX::XMFLOAT3> l_transformedCubeVerts;
	Transform m_transform;

	static float m_debugSphereMeshRadius;

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

	bool CheckSATCollision(const std::shared_ptr<Collider> other);

	bool CheckGJKCollision(const std::shared_ptr<Collider> other);
	DirectX::XMVECTOR CalcSupport(const DirectX::XMVECTOR& direction);
	bool DoSimplex(std::vector<DirectX::XMVECTOR>& supports, DirectX::XMVECTOR& direction);

	Transform* m_sphere;
	Transform* m_cube;

public:
	Collider(std::shared_ptr<Mesh> colliderMesh, Transform* parentTransform);
	Collider(std::shared_ptr<Mesh> colliderMesh, Transform* parentTransform, Transform* sphere, Transform* cube);
	~Collider();

	std::shared_ptr<Mesh> GetCollisionMesh() { return m_objectMesh; }

	bool CheckForCollision(const std::shared_ptr<Collider> other, bool overrideSphereCheck = false);
	bool CheckSphereColliding(const std::shared_ptr<Collider> other);
	void MakePointsDirty() { m_pointsDirty = true; }
	void MakeHalvesDirty() { m_pointsDirty = true; }

	void static SetDebugSphereMeshRadius(float newRadius) { m_debugSphereMeshRadius = newRadius; }
};

