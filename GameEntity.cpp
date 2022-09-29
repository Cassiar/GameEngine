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
}

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<RigidBody> rigidBody, std::shared_ptr<Collider> collider)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

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

	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();

	mesh->Draw();
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
		for (auto& entity : collisionEntities)
		{
			if (entity.get() != this && m_collider->CheckForCollision(entity->GetCollider())) {
				material->SetColorTint(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
				//this->GetTransform()->SetScale(2.0f, 2.0f, 2.0f);
				break;
			}
			else
			{
				material->SetColorTint(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
				//this->GetTransform()->SetScale(1.0f, 1.0f, 1.0f);
			}
		}
	}
}
