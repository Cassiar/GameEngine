#include "GameEntity.h"

#include <WICTextureLoader.h>

#include <string>
#include <codecvt>
#include <locale>

#include <map>

#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

bool g_drawDebugSpheresDefault = false;
bool g_drawDebugCubesDefault = true;

//Chris helper methods copied over
//including DXCore didn't seem to work
std::string GetExePath()
{
	// Assume the path is just the "current directory" for now
	std::string path = ".\\";

	// Get the real, full path to this executable
	char currentDir[1024] = {};
	GetModuleFileName(0, currentDir, 1024);

	// Find the location of the last slash charaacter
	char* lastSlash = strrchr(currentDir, '\\');
	if (lastSlash)
	{
		// End the string at the last slash character, essentially
		// chopping off the exe's file name.  Remember, c-strings
		// are null-terminated, so putting a "zero" character in 
		// there simply denotes the end of the string.
		*lastSlash = 0;

		// Set the remainder as the path
		path = currentDir;
	}

	// Toss back whatever we've found
	return path;
}


// ---------------------------------------------------
//  Same as GetExePath(), except it returns a wide character
//  string, which most of the Windows API requires.
// ---------------------------------------------------
std::wstring GetExePath_Wide()
{
	// Grab the path as a standard string
	std::string path = GetExePath();

	// Convert to a wide string
	wchar_t widePath[1024] = {};
	mbstowcs_s(0, widePath, path.c_str(), 1024);

	// Create a wstring for it and return
	return std::wstring(widePath);
}


// ----------------------------------------------------
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::string GetFullPathTo(std::string relativeFilePath)
{
	return GetExePath() + "\\" + relativeFilePath;
}



// ----------------------------------------------------
//  Same as GetFullPathTo, but with wide char strings.
// 
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath)
{
	return GetExePath_Wide() + L"\\" + relativeFilePath;
}

using namespace physx;

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, bool hasPhysics, bool isDebugEntity)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

	if (!isDebugEntity) {
		std::shared_ptr<AssetManager> assetManager = AssetManager::GetInstance();
		std::shared_ptr<Material> debugMat = std::make_shared<Material>(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f,
			assetManager->GetVertexShader("vertexShader"),
			assetManager->GetPixelShader("debugPixelShader"));

		std::shared_ptr<Material> debugMat2 = std::make_shared<Material>(DirectX::XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f,
			assetManager->GetVertexShader("vertexShader"),
			assetManager->GetPixelShader("debugPixelShader"));

		m_sphere = std::make_shared<GameEntity>(assetManager->GetMesh(3), debugMat, in_camera, false, true);
		m_cube = std::make_shared<GameEntity>(assetManager->GetMesh(0), debugMat2, in_camera, false, true);

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

		m_collider = std::make_shared<Collider>(in_mesh, &transform, m_sphere->GetTransform(), m_cube->GetTransform());
	}
	else
	{
		m_sphere = nullptr;
		m_cube = nullptr;
		m_collider = std::make_shared<Collider>(in_mesh, &transform);
	}

	if (hasPhysics && !m_rigidBody)
	{
		// Assumes dynamic. If User wants static they can pass it in
		std::shared_ptr<PhysXManager> instance = PhysXManager::GetInstance();
		DirectX::XMFLOAT3 halves = m_collider->GetHalfWidths();
		m_rigidBody = instance->CreateDynamic(PxTransform(0, 30, 0), PxBoxGeometry(halves.x,halves.y,halves.z));
	}
	else if(!hasPhysics)
	{
		m_rigidBody = nullptr;
	}

	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_drawDebugCube = g_drawDebugCubesDefault;
	m_isDebugEntity = isDebugEntity;
}

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, std::shared_ptr<PxRigidActor> rigidBody) 
	: GameEntity(in_mesh, in_material, in_camera, false, false)
{
	m_rigidBody = rigidBody;
}

GameEntity::GameEntity(std::shared_ptr<SabaMesh> in_mesh, std::shared_ptr<Camera> in_camera, Microsoft::WRL::ComPtr<ID3D11Device> in_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context,
	std::shared_ptr<SimpleVertexShader> vertexShader, std::shared_ptr<SimplePixelShader> pixelShader,
	std::shared_ptr<SimpleVertexShader> edgeVertexShader, std::shared_ptr<SimplePixelShader> edgePixelShader) 
	: GameEntity(in_mesh, nullptr, in_camera, false, false) {
	context = in_context;
	sabaMesh = in_mesh;
	m_collider = std::make_shared<Collider>(in_mesh, &transform);
	m_sphere = nullptr;
	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_drawDebugCube = g_drawDebugCubesDefault;
	m_isDebugEntity = false;

	if (mesh->IsPmx()) {
		DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
			GetFullPathTo_Wide(L"../../Assets/Textures/allMetal.png").c_str(), nullptr, one.GetAddressOf());
		DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
			GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, zero.GetAddressOf());
		DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
			GetFullPathTo_Wide(L"../../Assets/Textures/Ramp_Texture.png").c_str(), nullptr, rampTexture.GetAddressOf());
	}
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

void GameEntity::DrawPMX(DirectX::XMFLOAT4X4 world, DirectX::XMFLOAT4X4 view, DirectX::XMFLOAT4X4 projection,
	/*DirectX::XMFLOAT3 m_lightColor, DirectX::XMFLOAT3 m_lightDir,*/ int numLights, Light* lights,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView,
	float m_screenWidth, float m_screenHeight) {
	
	//create world view and world view proj mats
	DirectX::XMMATRIX wMat;
	DirectX::XMMATRIX vMat;
	DirectX::XMMATRIX pMat;

	DirectX::XMFLOAT4X4 wv;
	DirectX::XMFLOAT4X4 wvp;

	wMat = DirectX::XMLoadFloat4x4(&world);
	vMat = DirectX::XMLoadFloat4x4(&view);
	pMat = DirectX::XMLoadFloat4x4(&projection);

	DirectX::XMStoreFloat4x4(&wv,
		wMat * vMat);
	DirectX::XMStoreFloat4x4(&wvp,
		wMat * vMat * pMat);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, sabaMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(sabaMesh->GetIndexBuffer().Get(), sabaMesh->GetFormat(), 0);

	//from saba library example
	// 
	// Draw model
		// Set viewport
	D3D11_VIEWPORT vp;
	vp.Width = float(m_screenWidth);
	vp.Height = float(m_screenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	size_t subMeshCount = sabaMesh->GetModel()->GetSubMeshCount();
	std::shared_ptr<AssetManager> assetManager = AssetManager::GetInstance();
	std::vector<SabaMaterial> sabaMats = assetManager->GetSabaStructMaterials();
	std::vector<std::shared_ptr<Material>> materials = assetManager->GetSabaMaterials();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = sabaMesh->GetModel()->GetSubMeshes()[i];
		
		const auto& mat = sabaMats[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (mmdMat.m_alpha == 0)
		{
			continue;
		}

		DirectX::XMFLOAT4 texModes = {};
		
		materials[subMesh.m_materialID]->PrepareMaterial();

		//set vertex shader
		std::shared_ptr<SimpleVertexShader> vs = materials[subMesh.m_materialID]->GetVertexShader();
		std::shared_ptr<SimplePixelShader> ps = materials[subMesh.m_materialID]->GetPixelShader();

		vs->SetShader();
		vs->SetMatrix4x4("W", world);
		vs->SetMatrix4x4("WV", wv);
		vs->SetMatrix4x4("WVP", wvp);

		ps->SetShader();

		ps->SetFloat("Alpha", mmdMat.m_alpha);
		ps->SetFloat3("Diffuse", DirectX::XMFLOAT3(mmdMat.m_diffuse[0], mmdMat.m_diffuse[1], mmdMat.m_diffuse[2]));
		ps->SetFloat3("Ambient", DirectX::XMFLOAT3(mmdMat.m_ambient[0], mmdMat.m_ambient[1], mmdMat.m_ambient[2]));
		ps->SetFloat3("Specular", DirectX::XMFLOAT3(mmdMat.m_specular[0], mmdMat.m_specular[1], mmdMat.m_specular[2]));
		ps->SetFloat("SpecularPower", mmdMat.m_specularPower);

		if (mat.m_texture.m_texture)
		{
			if (!mat.m_texture.m_hasAlpha)
			{
				// Use Material Alpha
				texModes.x = 1;
			}
			else
			{
				// Use Material Alpha * Texture Alpha
				texModes.x = 2;
			}
			ps->SetFloat4("TexMulFactor", DirectX::XMFLOAT4(mmdMat.m_textureMulFactor[0], mmdMat.m_textureMulFactor[1], mmdMat.m_textureMulFactor[2], mmdMat.m_textureMulFactor[3]));
			ps->SetFloat4("TexAddFactor", DirectX::XMFLOAT4(mmdMat.m_textureAddFactor[0], mmdMat.m_textureAddFactor[1], mmdMat.m_textureAddFactor[2], mmdMat.m_textureAddFactor[3]));
			ps->SetShaderResourceView("Tex", mat.m_texture.m_textureView.Get());
			ps->SetSamplerState("TexSampler", assetManager->GetSampler("sabaSampler").Get());
		}
		else
		{
			texModes.x = 0;
			ps->SetFloat4("TexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetFloat4("TexAddFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetShaderResourceView("Tex", assetManager->m_dummyTextureView);
			ps->SetSamplerState("TexSampler", assetManager->m_dummySampler);
		}

		if (mat.m_toonTexture.m_texture)
		{
			texModes.y = 1;
			ps->SetFloat4("ToonTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureMulFactor[0], mmdMat.m_toonTextureMulFactor[1], mmdMat.m_toonTextureMulFactor[2], mmdMat.m_toonTextureMulFactor[3]));
			ps->SetFloat4("ToonTexAddFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureAddFactor[0], mmdMat.m_toonTextureAddFactor[1], mmdMat.m_toonTextureAddFactor[2], mmdMat.m_toonTextureAddFactor[3]));
			ps->SetShaderResourceView("ToonTex", mat.m_toonTexture.m_textureView);
			ps->SetSamplerState("ToonTexSampler", assetManager->m_toonTextureSampler);
		}
		else
		{
			texModes.y = 0; 
			ps->SetFloat4("ToonTexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetFloat4("ToonTexAddFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetShaderResourceView("ToonTex", assetManager->m_dummyTextureView);
			ps->SetSamplerState("ToonTexSampler", assetManager->m_dummySampler);
		}

		if (mat.m_spTexture.m_texture)
		{
			if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
			{
				//psCB.m_textureModes.z = 1;
				texModes.z = 1;
			}
			else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
			{
				texModes.z = 2;
			}
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureMulFactor[0], mmdMat.m_spTextureMulFactor[1], mmdMat.m_spTextureMulFactor[2], mmdMat.m_spTextureMulFactor[3]));
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureAddFactor[0], mmdMat.m_spTextureAddFactor[1], mmdMat.m_spTextureAddFactor[2], mmdMat.m_spTextureAddFactor[3]));
			ps->SetShaderResourceView("SphereTex", mat.m_spTexture.m_textureView);
			ps->SetSamplerState("SphereTexSampler", assetManager->m_sphereTextureSampler);
		}
		else
		{
			texModes.z = 0;
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetShaderResourceView("SphereTex", assetManager->m_dummyTextureView);
			ps->SetSamplerState("SphereTexSampler", assetManager->m_dummySampler);
		}

		//DirectX::XMFLOAT4 lightDir = DirectX::XMFLOAT4(m_lightDir.x, m_lightDir.y, m_lightDir.z, 1.0f);
		//DirectX::XMFLOAT4X4 viewMat = view;
		////update light dir to be in screen space
		//DirectX::XMStoreFloat4(&lightDir,
		//	DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&lightDir),
		//		DirectX::XMLoadFloat4x4(&viewMat)));

		//ps->SetFloat3("LightColor", m_lightColor);
		//ps->SetFloat3("LightDir", m_lightDir);
		ps->SetInt("numLights", numLights);
		ps->SetData("lights", &lights[0], sizeof(Light)* (int)numLights);

		ps->SetFloat4("TextureModes", texModes);

		vs->CopyAllBufferData();
		ps->CopyAllBufferData();


		if (mmdMat.m_bothFace)
		{
			context->RSSetState(assetManager->m_mmdBothFaceRS.Get());
		}
		else
		{
			context->RSSetState(assetManager->m_mmdFrontFaceRS.Get());
		}

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, sabaMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(sabaMesh->GetIndexBuffer().Get(), sabaMesh->GetFormat(), 0);

		context->OMSetBlendState(assetManager->m_mmdBlendState.Get(), nullptr, 0xffffffff);

		context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}
	
	{
		ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr };
		ID3D11SamplerState* samplers[] = { nullptr, nullptr, nullptr };
		context->PSSetShaderResources(0, 3, views);
		context->PSSetSamplers(0, 3, samplers);
	}

#pragma region Edge
	// Draw edge

	// Setup input assembler
	//{
	//	context->IASetInputLayout(assetManager->m_mmdEdgeInputLayout.Get());
	//}

	//materials = assetManager->GetSabaEdgeMaterials();

	//for (size_t i = 0; i < subMeshCount; i++)
	//{
	//	const auto& subMesh = sabaMesh->GetModel()->GetSubMeshes()[i];
	//	const auto& mat = sabaMats[subMesh.m_materialID];
	//	const auto& mmdMat = mat.m_mmdMat;

	//	if (!mmdMat.m_edgeFlag)
	//	{
	//		continue;
	//	}
	//	if (mmdMat.m_alpha == 0.0f)
	//	{
	//		continue;
	//	}


	//	//set vertex shader
	//	std::shared_ptr<SimpleVertexShader> vs = materials[subMesh.m_materialID]->GetVertexShader();
	//	std::shared_ptr<SimplePixelShader> ps = materials[subMesh.m_materialID]->GetPixelShader();

	//	vs->SetShader();
	//	vs->SetMatrix4x4("W", world);
	//	vs->SetMatrix4x4("WV", wv);
	//	vs->SetMatrix4x4("WVP", wvp);
	//	vs->SetFloat2("ScreenSize", DirectX::XMFLOAT2(m_screenWidth, m_screenHeight));
	//	vs->SetFloat("EdgeSize", mmdMat.m_edgeSize/1000.0f);

	//	ps->SetShader();

	//	ps->SetFloat4("EdgeColor", DirectX::XMFLOAT4(mmdMat.m_edgeColor.x, mmdMat.m_edgeColor.y, mmdMat.m_edgeColor.z, mmdMat.m_edgeColor.w));

	//	vs->CopyAllBufferData();
	//	ps->CopyAllBufferData();
	//	context->RSSetState(assetManager->m_mmdEdgeRS.Get());

	//	context->OMSetBlendState(assetManager->m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

	//	context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	//}

	//end saba lib example
#pragma endregion
}


void GameEntity::Update(float dt, std::vector<std::shared_ptr<GameEntity>>& collisionEntities)
{
	if (m_rigidBody)
	{
		transform.SetTransformsFromPhysX(m_rigidBody->getGlobalPose());
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
