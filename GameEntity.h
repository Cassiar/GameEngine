//Author: Cassiar Beaver
//this class represents a game object on the screen.
//This handles drawing object as well as transforming it.
//multiple objects can share the same mesh.
#pragma once
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory> //for shared pointers
#include <DirectXMath.h>

#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Collider.h"
#include "Material.h"
#include "RigidBody.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, bool isDebugSphere = false);
	GameEntity(std::shared_ptr <Mesh> in_mesh, std::shared_ptr<Camera> in_camera, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<GameEntity> sphere, Microsoft::WRL::ComPtr<ID3D11Device> device);
	GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<RigidBody> rigidBody, std::shared_ptr<Collider> collider);
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

	bool IsDebugSphere() { return m_isDebugSphere; }

	bool ShouldDrawSphere() { return m_drawDebugSphere; }
	void SetDrawSphere(bool drawDebugSphere) { m_drawDebugSphere = drawDebugSphere; }

	//will hold draw code
	void Draw();
	void DrawPMX(DirectX::XMFLOAT4X4 world, DirectX::XMFLOAT4X4 view, DirectX::XMFLOAT4X4 projection, DirectX::XMFLOAT3 m_lightColor, DirectX::XMFLOAT3 m_lightDir, float m_screenWidth, float m_screenHeight);
	//Updates entity and checks for collisions
	void Update(float dt, std::vector<std::shared_ptr<GameEntity>>& collisionEntities);
private:
	bool CreateSabaShaders();

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Camera> camera;
	Transform transform;
	std::shared_ptr<Material> material;

	std::shared_ptr<RigidBody> m_rigidBody;
	std::shared_ptr<Collider> m_collider;

	std::shared_ptr<GameEntity> m_sphere;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_debugRastState;
	bool m_drawDebugSphere;
	bool m_isDebugSphere;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};