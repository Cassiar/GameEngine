#include "GameEntity.h"

#include "BufferStructs.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

	m_rigidBody = std::make_shared<RigidBody>(&transform);
	m_collider = std::make_shared<Collider>(in_mesh, &transform);
	m_sphere = nullptr;
}

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<GameEntity> sphere, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

	m_rigidBody = std::make_shared<RigidBody>(&transform);
	m_collider = std::make_shared<Collider>(in_mesh, &transform, sphere->GetTransform());

	//create rasterizer state
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_WIREFRAME;
	shadowRastDesc.CullMode = D3D11_CULL_NONE;
	shadowRastDesc.DepthClipEnable = true;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rast;// = sphere->GetRastState();
	device->CreateRasterizerState(&shadowRastDesc, rast.GetAddressOf());
	sphere->SetDebugRast(rast);

	m_sphere = sphere;
}

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<RigidBody> rigidBody, std::shared_ptr<Collider> collider)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

	m_rigidBody = rigidBody;
	m_collider = collider;
	m_sphere = nullptr;
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

	if (m_sphere) {
		//m_sphere->GetTransform()->SetScale(1.5f, 1.5f, 1.5f);
		m_sphere->Draw();
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
		bool colliding = false;
		for (auto& entity : collisionEntities)
		{
			if (entity.get() != this && m_collider->CheckForCollision(entity->GetCollider())) {
				colliding = true;
				break;
			}
		}

		//Debug collision code
		if (m_sphere) {
			if (colliding)
			{
				m_sphere->GetMaterial()->SetColorTint(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
			}
			else
			{
				m_sphere->GetMaterial()->SetColorTint(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f));
			}
		}
	}
}
