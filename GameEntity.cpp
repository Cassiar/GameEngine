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
	sabaMesh = std::shared_ptr<SabaMesh>(dynamic_cast<SabaMesh*>(this->mesh.get()));
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

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Camera> in_camera, Microsoft::WRL::ComPtr<ID3D11Device> in_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context,
	std::shared_ptr<SimpleVertexShader> vertexShader, std::shared_ptr<SimplePixelShader> pixelShader,
	std::shared_ptr<SimpleVertexShader> edgeVertexShader, std::shared_ptr<SimplePixelShader> edgePixelShader) 
	: GameEntity(in_mesh, nullptr, in_camera, false, false) {
	context = in_context;
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
	DirectX::XMFLOAT3 m_lightColor, DirectX::XMFLOAT3 m_lightDir,
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
	context->IASetVertexBuffers(0, 1, mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetFormat(), 0);
	//testing uvs 
	//for (int i = 0; i < mesh->GetVerticies().size(); i++) {
	//	Vertex temp = mesh->GetVerticies()[i];
	//	printf("i: %d \tU: %f, \tV: %f\n", i, temp.UVCoord.x, temp.UVCoord.y);
	//}

	//size_t subMeshCount = mesh->GetModel()->GetSubMeshCount();
	//for (size_t i = 0; i < subMeshCount; i++)
	//{
	//	const auto& subMesh = mesh->GetModel()->GetSubMeshes()[i];
	//	const auto& mat = m_materials[subMesh.m_materialID];
	//	const auto& mmdMat = mat.m_mmdMat;

	//	if (mmdMat.m_alpha == 0)
	//	{
	//		continue;
	//	}

	//	materials[subMesh.m_materialID]->PrepareMaterial();

	//	std::shared_ptr<SimpleVertexShader> vs = materials[subMesh.m_materialID]->GetVertexShader();
	//	std::shared_ptr<SimplePixelShader> ps = materials[subMesh.m_materialID]->GetPixelShader();

	//	materials[subMesh.m_materialID]->GetVertexShader()->SetShader();
	//	materials[subMesh.m_materialID]->GetPixelShader()->SetShader();
	//	//    float4x4 WV;
	//	//	  float4x4 WVP;

	//	//set the values for the vertex shader
	//	//string names MUST match those in VertexShader.hlsl
	//	//vs->SetMatrix4x4("world", world);
	//	vs->SetMatrix4x4("WV", wv);
	//	vs->SetMatrix4x4("WVP", wvp);
	//	//set pixel shader buffer values
	//	//
	//	//float   Alpha;
	//	//float3  Diffuse;
	//	//float3  Ambient;
	//	//float3  Specular;
	//	//float   SpecularPower;
	//	//float3  LightColor;
	//	//float3  LightDir;
	//	//
	//	//float4  TexMulFactor;
	//	//float4  TexAddFactor;
	//	//
	//	//float4  ToonTexMulFactor;
	//	//float4  ToonTexAddFactor;
	//	//
	//	//float4  SphereTexMulFactor;
	//	//float4  SphereTexAddFactor;
	//	//
	//	//int4    TextureModes;

	//	int texMode[4] = {};

	//	if (mat.m_texture.m_texture)
	//	{
	//		if (!mat.m_texture.m_hasAlpha)
	//		{
	//			// Use Material Alpha
	//			texMode[0] = 1;
	//		}
	//		else
	//		{
	//			// Use Material Alpha * Texture Alpha
	//			texMode[0] = 2;
	//		}
	//	}
	//	else {
	//		texMode[0] = 0;
	//	}
	//	if (mat.m_toonTexture.m_texture)
	//	{
	//		texMode[1] = 1;
	//	}
	//	else {
	//		texMode[1] = 0;
	//	}
	//	if (mat.m_spTexture.m_texture)
	//	{
	//		if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
	//		{
	//			texMode[2] = 1;
	//		}
	//		else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
	//		{
	//			texMode[2] = 2;
	//		}
	//	}
	//	else {
	//		texMode[2] = 0;
	//	}

	//	ps->SetFloat("Alpha", mmdMat.m_alpha);
	//	ps->SetFloat3("Diffuse", DirectX::XMFLOAT3(mmdMat.m_diffuse[0], mmdMat.m_diffuse[1], mmdMat.m_diffuse[2]));
	//	ps->SetFloat3("Ambient", DirectX::XMFLOAT3(mmdMat.m_ambient[0], mmdMat.m_ambient[1], mmdMat.m_ambient[2]));
	//	ps->SetFloat3("Specular", DirectX::XMFLOAT3(mmdMat.m_specular[0], mmdMat.m_specular[1], mmdMat.m_specular[2]));
	//	ps->SetFloat("SpecularPower", mmdMat.m_specularPower);
	//	ps->SetFloat3("LightColor", m_lightColor);
	//	ps->SetFloat3("LightDir", m_lightDir);

	//	ps->SetFloat4("TexMulFactor", DirectX::XMFLOAT4(mmdMat.m_textureMulFactor[0], mmdMat.m_textureMulFactor[1], mmdMat.m_textureMulFactor[2], mmdMat.m_textureMulFactor[3]));
	//	ps->SetFloat4("TexAddFactor", DirectX::XMFLOAT4(mmdMat.m_textureAddFactor[0], mmdMat.m_textureAddFactor[1], mmdMat.m_textureAddFactor[2], mmdMat.m_textureAddFactor[3]));

	//	ps->SetFloat4("ToonTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureMulFactor[0], mmdMat.m_toonTextureMulFactor[1], mmdMat.m_toonTextureMulFactor[2], mmdMat.m_toonTextureMulFactor[3]));
	//	ps->SetFloat4("ToonTexAddFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureAddFactor[0], mmdMat.m_toonTextureAddFactor[1], mmdMat.m_toonTextureAddFactor[2], mmdMat.m_toonTextureAddFactor[3]));

	//	ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureMulFactor[0], mmdMat.m_spTextureMulFactor[1], mmdMat.m_spTextureMulFactor[2], mmdMat.m_spTextureMulFactor[3]));
	//	ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureAddFactor[0], mmdMat.m_spTextureAddFactor[1], mmdMat.m_spTextureAddFactor[2], mmdMat.m_spTextureAddFactor[3]));

	//	ps->SetData("TextureModes", texMode, sizeof(texMode));
	//	//copy data over to gpu. Equivelent to map, memcpy, unmap
	//	vs->CopyAllBufferData();
	//	ps->CopyAllBufferData();


	//	// Finally do the actual drawing
	//	// Once per object
	//	context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	//}

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

	//context->RSSetViewports(1, &vp);
	//ID3D11RenderTargetView* rtvs[] = { m_renderTargetView.Get() };
	//context->OMSetRenderTargets(1, rtvs,
	//	m_depthStencilView.Get()
	//);

	//context->OMSetDepthStencilState(m_defaultDSS.Get(), 0x00);

	// Setup input assembler
	{
		//UINT strides[] = { sizeof(Vertex) };
		//UINT offsets[] = { 0 };
		//context->IASetInputLayout(m_mmdInputLayout.Get());
		//ID3D11Buffer* vbs[] = { m_vertexBuffer.Get() };
		//context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
		//context->IASetIndexBuffer(m_indexBuffer.Get(), m_indexBufferFormat, 0);
		//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	
	// Setup vertex shader
	//{
	//	MMDVertexShaderCB vsCB;
	//	vsCB.m_wv = wv;
	//	vsCB.m_wvp = wvp;
	//	context->UpdateSubresource(m_mmdVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

	//	// Vertex shader
	//	context->VSSetShader(m_mmdVS.Get(), nullptr, 0);
	//	ID3D11Buffer* cbs[] = { m_mmdVSConstantBuffer.Get() };
	//	context->VSSetConstantBuffers(0, 1, cbs);
	//}

	size_t subMeshCount = sabaMesh->GetModel()->GetSubMeshCount();
	std::shared_ptr<AssetManager> assetManager = AssetManager::GetInstance();
	std::vector<SabaMaterial> sabaMats = assetManager->GetSabaStructMaterials();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = sabaMesh->GetModel()->GetSubMeshes()[i];
		
		std::vector<std::shared_ptr<Material>> materials = assetManager->GetSabaMaterials();
		const auto& mat = sabaMats[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (mmdMat.m_alpha == 0)
		{
			continue;
		}
		
		materials[subMesh.m_materialID]->PrepareMaterial();

		//set vertex shader
		std::shared_ptr<SimpleVertexShader> vs = materials[subMesh.m_materialID]->GetVertexShader();
		std::shared_ptr<SimplePixelShader> ps = materials[subMesh.m_materialID]->GetPixelShader();

		vs->SetShader();
		vs->SetMatrix4x4("WV", wv);
		vs->SetMatrix4x4("WVP", wvp);
		
		// Pixel shader
		//context->PSSetShader(m_mmdPS.Get(), nullptr, 0);

		ps->SetShader();

		MMDPixelShaderCB psCB = {};
		psCB.m_alpha = mmdMat.m_alpha;
		psCB.m_diffuse = mmdMat.m_diffuse;
		psCB.m_ambient = mmdMat.m_ambient;
		psCB.m_specular = mmdMat.m_specular;
		psCB.m_specularPower = mmdMat.m_specularPower;

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
				psCB.m_textureModes.x = 1;
			}
			else
			{
				// Use Material Alpha * Texture Alpha
				psCB.m_textureModes.x = 2;
			}
			psCB.m_texMulFactor = mmdMat.m_textureMulFactor;
			psCB.m_texAddFactor = mmdMat.m_textureAddFactor;
			//ID3D11ShaderResourceView* views[] = { mat.m_texture.m_textureView.Get() };
			//ID3D11SamplerState* samplers[] = { m_textureSampler.Get() };
			//context->PSSetShaderResources(0, 1, views);
			//context->PSSetSamplers(0, 1, samplers);
			ps->SetFloat4("TexMulFactor", DirectX::XMFLOAT4(mmdMat.m_textureMulFactor[0], mmdMat.m_textureMulFactor[1], mmdMat.m_textureMulFactor[2], mmdMat.m_textureMulFactor[3]));
			ps->SetFloat4("TexAddFactor", DirectX::XMFLOAT4(mmdMat.m_textureAddFactor[0], mmdMat.m_textureAddFactor[1], mmdMat.m_textureAddFactor[2], mmdMat.m_textureAddFactor[3]));
			ps->SetShaderResourceView("Tex", mat.m_texture.m_textureView.Get());
			ps->SetSamplerState("TexSampler", assetManager->GetSampler("sabaSampler").Get());
		}
		else
		{
			psCB.m_textureModes.x = 0;
			//ID3D11ShaderResourceView* views[] = { m_dummyTextureView.Get() };
			//ID3D11SamplerState* samplers[] = { m_dummySampler.Get() };
			//context->PSSetShaderResources(0, 1, views);
			//context->PSSetSamplers(0, 1, samplers);
			ps->SetFloat4("TexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetFloat4("TexAddFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetShaderResourceView("Tex", assetManager->m_dummyTextureView);
			ps->SetSamplerState("TexSampler", assetManager->m_dummySampler);
		}

		if (mat.m_toonTexture.m_texture)
		{
			psCB.m_textureModes.y = 1;
			psCB.m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
			psCB.m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
			//ID3D11ShaderResourceView* views[] = { mat.m_toonTexture.m_textureView.Get() };
			//ID3D11SamplerState* samplers[] = { m_toonTextureSampler.Get() };
			//context->PSSetShaderResources(1, 1, views);
			//context->PSSetSamplers(1, 1, samplers);
			ps->SetFloat4("ToonTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureMulFactor[0], mmdMat.m_toonTextureMulFactor[1], mmdMat.m_toonTextureMulFactor[2], mmdMat.m_toonTextureMulFactor[3]));
			ps->SetFloat4("ToonTexAddFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureAddFactor[0], mmdMat.m_toonTextureAddFactor[1], mmdMat.m_toonTextureAddFactor[2], mmdMat.m_toonTextureAddFactor[3]));
			ps->SetShaderResourceView("ToonTex", mat.m_toonTexture.m_textureView);
			ps->SetSamplerState("ToonTexSampler", assetManager->m_toonTextureSampler);
		}
		else
		{
			psCB.m_textureModes.y = 0;
			//ID3D11ShaderResourceView* views[] = { m_dummyTextureView.Get() };
			//ID3D11SamplerState* samplers[] = { m_dummySampler.Get() };
			//context->PSSetShaderResources(1, 1, views);
			//context->PSSetSamplers(1, 1, samplers);
			ps->SetFloat4("ToonTexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetFloat4("ToonTexAddFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetShaderResourceView("ToonTex", assetManager->m_dummyTextureView);
			ps->SetSamplerState("ToonTexSampler", assetManager->m_dummySampler);
		}

		if (mat.m_spTexture.m_texture)
		{
			if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
			{
				psCB.m_textureModes.z = 1;
			}
			else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
			{
				psCB.m_textureModes.z = 2;
			}
			psCB.m_sphereTexMulFactor = mmdMat.m_spTextureMulFactor;
			psCB.m_sphereTexAddFactor = mmdMat.m_spTextureAddFactor;
			//ID3D11ShaderResourceView* views[] = { mat.m_spTexture.m_textureView.Get() };
			//ID3D11SamplerState* samplers[] = { m_sphereTextureSampler.Get() };
			//context->PSSetShaderResources(2, 1, views);
			//context->PSSetSamplers(2, 1, samplers);
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureMulFactor[0], mmdMat.m_spTextureMulFactor[1], mmdMat.m_spTextureMulFactor[2], mmdMat.m_spTextureMulFactor[3]));
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureAddFactor[0], mmdMat.m_spTextureAddFactor[1], mmdMat.m_spTextureAddFactor[2], mmdMat.m_spTextureAddFactor[3]));
			ps->SetShaderResourceView("SphereTex", mat.m_spTexture.m_textureView);
			ps->SetSamplerState("SphereTexSampler", assetManager->m_sphereTextureSampler);
		}
		else
		{
			psCB.m_textureModes.z = 0;
			//ID3D11ShaderResourceView* views[] = { m_dummyTextureView.Get() };
			//ID3D11SamplerState* samplers[] = { m_dummySampler.Get() };
			//context->PSSetShaderResources(2, 1, views);
			//context->PSSetSamplers(2, 1, samplers);			
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(0,0,0,0));
			ps->SetShaderResourceView("SphereTex", assetManager->m_dummyTextureView);
			ps->SetSamplerState("SphereTexSampler", assetManager->m_dummySampler);
		}

		psCB.m_lightColor = glm::vec3(m_lightColor.x, m_lightColor.y, m_lightColor.z);
		DirectX::XMFLOAT4 lightDir = DirectX::XMFLOAT4(m_lightDir.x, m_lightDir.y, m_lightDir.z, 1.0f);
		DirectX::XMFLOAT4X4 viewMat = view;
		//lightDir = viewMat * lightDir;
		//update light dir to be in screen space
		DirectX::XMStoreFloat4(&lightDir,
			DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&lightDir),
				DirectX::XMLoadFloat4x4(&viewMat)));
		psCB.m_lightDir = glm::vec3(lightDir.x, lightDir.y, lightDir.z);

		ps->SetFloat3("LightColor", m_lightColor);
		ps->SetFloat3("LightDir", m_lightDir);

		ps->SetFloat4("TextureModes", DirectX::XMFLOAT4(psCB.m_textureModes.x, psCB.m_textureModes.y, psCB.m_textureModes.z, psCB.m_textureModes.w));

		vs->CopyAllBufferData();
		ps->CopyAllBufferData();

		//context->UpdateSubresource(m_mmdPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);
		//ID3D11Buffer* pscbs[] = { m_mmdPSConstantBuffer.Get() };
		//context->PSSetConstantBuffers(1, 1, pscbs);

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
		context->IASetVertexBuffers(0, 1, mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetFormat(), 0);

		context->OMSetBlendState(assetManager->m_mmdBlendState.Get(), nullptr, 0xffffffff);

		context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}
	
	{
		ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr };
		ID3D11SamplerState* samplers[] = { nullptr, nullptr, nullptr };
		context->PSSetShaderResources(0, 3, views);
		context->PSSetSamplers(0, 3, samplers);
	}

	// Draw edge

	// Setup input assembler
	{
		context->IASetInputLayout(assetManager->m_mmdEdgeInputLayout.Get());
	}

	// Setup vertex shader (VSData)
	//{
	//	MMDEdgeVertexShaderCB vsCB;
	//	vsCB.m_wv = wv;
	//	vsCB.m_wvp = wvp;
	//	vsCB.m_screenSize = glm::vec2(float(m_screenWidth), float(m_screenHeight));
	//	context->UpdateSubresource(m_mmdEdgeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

	//	// Vertex shader
	//	context->VSSetShader(m_mmdEdgeVS.Get(), nullptr, 0);
	//	ID3D11Buffer* cbs[] = { m_mmdEdgeVSConstantBuffer.Get() };
	//	context->VSSetConstantBuffers(0, 1, cbs);
	//}

	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = sabaMesh->GetModel()->GetSubMeshes()[i];
		const auto& mat = sabaMats[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (!mmdMat.m_edgeFlag)
		{
			continue;
		}
		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		std::vector<std::shared_ptr<Material>> materials = assetManager->GetSabaMaterials();

		//set vertex shader
		std::shared_ptr<SimpleVertexShader> vs = materials[subMesh.m_materialID]->GetVertexShader();
		std::shared_ptr<SimplePixelShader> ps = materials[subMesh.m_materialID]->GetPixelShader();

		vs->SetShader();
		vs->SetMatrix4x4("WV", wv);
		vs->SetMatrix4x4("WVP", wvp);
		vs->SetFloat2("ScreenSize", DirectX::XMFLOAT2(m_screenWidth, m_screenHeight));
		vs->SetFloat("EdgeSize", mmdMat.m_edgeSize);

		// Pixel shader
		//context->PSSetShader(m_mmdPS.Get(), nullptr, 0);

		ps->SetShader();

		// Edge size constant buffer
		//{
		//	MMDEdgeSizeVertexShaderCB vsCB;
		//	vsCB.m_edgeSize = mmdMat.m_edgeSize;
		//	context->UpdateSubresource(m_mmdEdgeSizeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

		//	ID3D11Buffer* cbs[] = { m_mmdEdgeSizeVSConstantBuffer.Get() };
		//	context->VSSetConstantBuffers(1, 1, cbs);
		//}

		ps->SetFloat4("EdgeColor", DirectX::XMFLOAT4(mmdMat.m_edgeColor.x, mmdMat.m_edgeColor.y, mmdMat.m_edgeColor.z, mmdMat.m_edgeColor.w));
		// Pixel shader
		//context->PSSetShader(m_mmdEdgePS.Get(), nullptr, 0);
		//{
		//	MMDEdgePixelShaderCB psCB;
		//	psCB.m_edgeColor = mmdMat.m_edgeColor;
		//	context->UpdateSubresource(m_mmdEdgePSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

		//	ID3D11Buffer* pscbs[] = { m_mmdEdgePSConstantBuffer.Get() };
		//	context->PSSetConstantBuffers(2, 1, pscbs);
		//}

		context->RSSetState(assetManager->m_mmdEdgeRS.Get());

		context->OMSetBlendState(assetManager->m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

		context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}

	//end saba lib example
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
