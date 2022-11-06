#include "GameEntity.h"

#include "BufferStructs.h"

bool g_drawDebugSpheresDefault = true;
bool g_drawDebugCubesDefault = true;

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, bool isDebugEntity)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

	m_rigidBody = std::make_shared<RigidBody>(&transform);

	if (!isDebugEntity) {
		std::shared_ptr<AssetManager> assetManager = AssetManager::GetInstance();
		std::shared_ptr<Material> debugMat = std::make_shared<Material>(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f,
			assetManager->GetVertexShader("vertexShader"),
			assetManager->GetPixelShader("debugPixelShader"));

		std::shared_ptr<Material> debugMat2 = std::make_shared<Material>(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f,
			assetManager->GetVertexShader("vertexShader"),
			assetManager->GetPixelShader("debugPixelShader"));

		m_sphere = std::make_shared<GameEntity>(assetManager->GetMesh(3), debugMat, in_camera, true);
		m_cube = std::make_shared<GameEntity>(assetManager->GetMesh(0), debugMat2, in_camera, true);

		//create rasterizer state
		D3D11_RASTERIZER_DESC wireFrameRastDesc = {};
		wireFrameRastDesc.FillMode = D3D11_FILL_WIREFRAME;
		wireFrameRastDesc.CullMode = D3D11_CULL_NONE;
		wireFrameRastDesc.DepthClipEnable = true;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> rast;
		assetManager->MakeRasterizerState(wireFrameRastDesc, rast);
		m_sphere->SetDebugRast(rast);
		m_cube->SetDebugRast(rast);

		transform.AddChild(m_sphere->GetTransform());
		transform.AddChild(m_cube->GetTransform());

		m_collider = std::make_shared<Collider>(in_mesh, &transform, m_sphere->GetTransform(), m_cube->GetTransform());
	}
	else
	{
		m_sphere = nullptr;
		m_cube = nullptr;
		m_collider = std::make_shared<Collider>(in_mesh, &transform);
	}

	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_drawDebugSphere = g_drawDebugCubesDefault;
	m_isDebugEntity = isDebugEntity;
}

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<RigidBody> rigidBody, std::shared_ptr<Collider> collider) 
	: GameEntity(in_mesh, in_material, in_camera, false)
{
	m_rigidBody = rigidBody;
	m_collider = collider;
}

GameEntity::~GameEntity()
{
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

Transform* GameEntity::GetTransform()
{
	return &transform;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> in_material)
{
	material = in_material;
}

std::shared_ptr<Material> GameEntity::GetMaterial()
{
	return material;
}

void GameEntity::Draw()
{
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();

	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();

	//set the values for the vertex shader
	//string names MUST match those in VertexShader.hlsl
	vs->SetMatrix4x4("world", transform.GetWorldMatrix());
	vs->SetMatrix4x4("worldInvTranspose", transform.GetWorldInverseTransposeMatrix());
	vs->SetMatrix4x4("view", camera->GetViewMatrix());
	vs->SetMatrix4x4("proj", camera->GetProjectionMatrix());
	//set pixel shader buffer values
	DirectX::XMFLOAT4 color = material->GetColorTint();
	ps->SetFloat4("colorTint", color);
	ps->SetFloat3("cameraPos", camera->GetTransform()->GetPosition());
	ps->SetFloat("roughness", material->GetRoughness());
	
	//copy data over to gpu. Equivelent to map, memcpy, unmap
	vs->CopyAllBufferData();
	ps->CopyAllBufferData();


	if (m_debugRastState)
	{
		mesh->Draw(m_debugRastState);
	}
	else
	{
		mesh->Draw();
	}

	if (m_drawDebugSphere && m_sphere) {
		m_sphere->Draw();
	}
	if (m_drawDebugCube && m_cube) {
		m_cube->Draw();
	}
}

void GameEntity::Update(float dt, std::vector<std::shared_ptr<GameEntity>>& collisionEntities)
{
	if (m_rigidBody)
	{
		m_rigidBody->UpdateTransform(dt);
	}

	// Implement a singleton collision manager allowing for ease of collision checks
	if (m_collider)
	{
		bool sphereColliding = false;
		bool colliding = false;
		for (auto& entity : collisionEntities)
		{

			if (entity.get() != this) {
				sphereColliding = m_collider->CheckSphereColliding(entity->GetCollider());

				// Only really for debug purposes tho so this should be preprocessored out
				if (sphereColliding && m_collider->CheckForCollision(entity->GetCollider(), true)) {
					colliding = true;
					break;
				}
			}
		}

		//Debug collision code
		if (m_sphere) {
			if (sphereColliding)
			{
				m_sphere->GetMaterial()->SetColorTint(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
			}
			else
			{
				m_sphere->GetMaterial()->SetColorTint(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f));
			}
		}
		if (m_cube) {
			if (colliding)
			{
				m_cube->GetMaterial()->SetColorTint(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
			}
			else
			{
				m_cube->GetMaterial()->SetColorTint(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f));
			}
		}
	}
}
