//Author: Cassiar Beaver
//this class represents a game object on the screen.
//This handles drawing object as well as transforming it.
//multiple objects can share the same mesh.
#pragma once
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory> //for shared pointers
#include <DirectXMath.h>

#include "AssetManager.h"
#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Collider.h"
#include "Material.h"
#include "PhysXManager.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, bool hasPhysics = false, bool isDebugEntity = false);
	GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<physx::PxRigidActor> rigidBody);
	~GameEntity();

	//get pointer to mesh
	std::shared_ptr<Mesh> GetMesh();
	//get pointer to transform to allow changes outside
	Transform* GetTransform();
	std::shared_ptr<Collider> GetCollider() { return m_collider; }

	//get pointer to material 
	std::shared_ptr<Material> GetMaterial();
	//change the material
	void SetMaterial(std::shared_ptr<Material> in_material);

	void SetDebugRast(Microsoft::WRL::ComPtr<ID3D11RasterizerState> custRast) { m_debugRastState = custRast; }
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> GetRastState() { return m_debugRastState; }

	bool IsDebugSphere() { return m_isDebugEntity; }

	bool ShouldDrawSphere() { return m_drawDebugSphere; }
	void SetDrawSphere(bool drawDebugSphere) { m_drawDebugSphere = drawDebugSphere; }

	bool ShouldDrawCube() { return m_drawDebugCube; }
	void SetDrawCube(bool drawDebugCube) { m_drawDebugCube = drawDebugCube; }

	//will hold draw code
	void Draw();
	//Updates entity and checks for collisions
	void Update(float dt, std::vector<std::shared_ptr<GameEntity>>& collisionEntities);
private:

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Camera> camera;
	Transform transform;
	std::shared_ptr<Material> material;

	//Generic so that we can declare it as either a PxRigidDynamic or a PxRigidStatic
	std::shared_ptr<physx::PxRigidActor> m_rigidBody;
	std::shared_ptr<Collider> m_collider;

	std::shared_ptr<GameEntity> m_sphere;
	std::shared_ptr<GameEntity> m_cube;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_debugRastState;
	bool m_isDebugEntity;
	bool m_drawDebugSphere;
	bool m_drawDebugCube;
};