#include "GameEntity.h"

#include "BufferStructs.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

bool g_drawDebugSpheresDefault = true;
// mmd shader constant buffer

//========================
//Saba Code
//========================
struct MMDVertexShaderCB
{
	glm::mat4	m_wv;
	glm::mat4	m_wvp;
};

struct MMDPixelShaderCB
{
	float		m_alpha;
	glm::vec3	m_diffuse;
	glm::vec3	m_ambient;
	float		m_dummy1;
	glm::vec3	m_specular;
	float		m_specularPower;
	glm::vec3	m_lightColor;
	float		m_dummy2;
	glm::vec3	m_lightDir;
	float		m_dummy3;

	glm::vec4	m_texMulFactor;
	glm::vec4	m_texAddFactor;

	glm::vec4	m_toonTexMulFactor;
	glm::vec4	m_toonTexAddFactor;

	glm::vec4	m_sphereTexMulFactor;
	glm::vec4	m_sphereTexAddFactor;

	glm::ivec4	m_textureModes;

};

// mmd edge shader constant buffer

struct MMDEdgeVertexShaderCB
{
	glm::mat4	m_wv;
	glm::mat4	m_wvp;
	glm::vec2	m_screenSize;
	float		m_dummy[2];
};

struct MMDEdgeSizeVertexShaderCB
{
	float		m_edgeSize;
	float		m_dummy[3];
};

struct MMDEdgePixelShaderCB
{
	glm::vec4	m_edgeColor;
};


// mmd ground shadow shader constant buffer

struct MMDGroundShadowVertexShaderCB
{
	glm::mat4	m_wvp;
};

struct MMDGroundShadowPixelShaderCB
{
	glm::vec4	m_shadowColor;
};

struct Texture
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Texture2D>				m_texture;
	ComPtr<ID3D11ShaderResourceView>	m_textureView;
	bool								m_hasAlpha;
};

//========================
//End Saba Code
//========================

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera, bool isDebugSphere)
{
	mesh = in_mesh;
	material = in_material;
	camera = in_camera;
	transform = Transform();

	m_rigidBody = std::make_shared<RigidBody>(&transform);
	m_collider = std::make_shared<Collider>(in_mesh, &transform);
	m_sphere = nullptr;
	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_isDebugSphere = isDebugSphere;

	if (mesh->IsPmx()) {
		CreateSabaShaders();
	}
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
	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_isDebugSphere = false;

	if (mesh->IsPmx()) {
		CreateSabaShaders();
	}
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
	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_isDebugSphere = false;

	if (mesh->IsPmx()) {
		CreateSabaShaders();
	}
}


void GameEntity::AddDeviceContext(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	this->device = device;
	this->context = context;
}

void GameEntity::CreateSabaShaders() {
	HRESULT hr;

	// mmd shader
	hr = device->CreateVertexShader(mmd_vso_data, sizeof(mmd_vso_data), nullptr, &m_mmdVS);
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreatePixelShader(mmd_pso_data, sizeof(mmd_pso_data), nullptr, &m_mmdPS);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC mmdInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = device->CreateInputLayout(
		mmdInputElementDesc, 3,
		mmd_vso_data, sizeof(mmd_vso_data),
		&m_mmdInputLayout
	);
	if (FAILED(hr))
	{
		return false;
	}

	// Texture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = device->CreateSamplerState(&samplerDesc, &m_textureSampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// ToonTexture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = device->CreateSamplerState(&samplerDesc, &m_toonTextureSampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// SphereTexture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = device->CreateSamplerState(&samplerDesc, &m_sphereTextureSampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Blend State
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = device->CreateBlendState(&blendDesc, &m_mmdBlendState);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State (Front face)
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_BACK;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = device->CreateRasterizerState(&rsDesc, &m_mmdFrontFaceRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State (Both face)
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = device->CreateRasterizerState(&rsDesc, &m_mmdBothFaceRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// mmd edge shader
	hr = device->CreateVertexShader(mmd_edge_vso_data, sizeof(mmd_edge_vso_data), nullptr, &m_mmdEdgeVS);
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreatePixelShader(mmd_edge_pso_data, sizeof(mmd_edge_pso_data), nullptr, &m_mmdEdgePS);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC mmdEdgeInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = device->CreateInputLayout(
		mmdEdgeInputElementDesc, 2,
		mmd_edge_vso_data, sizeof(mmd_edge_vso_data),
		&m_mmdEdgeInputLayout
	);
	if (FAILED(hr))
	{
		return false;
	}

	// Blend State
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = device->CreateBlendState(&blendDesc, &m_mmdEdgeBlendState);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_FRONT;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = device->CreateRasterizerState(&rsDesc, &m_mmdEdgeRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// mmd ground shadow shader
	hr = device->CreateVertexShader(mmd_ground_shadow_vso_data, sizeof(mmd_ground_shadow_vso_data), nullptr, &m_mmdGroundShadowVS);
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreatePixelShader(mmd_ground_shadow_pso_data, sizeof(mmd_ground_shadow_pso_data), nullptr, &m_mmdGroundShadowPS);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC mmdGroundShadowInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = device->CreateInputLayout(
		mmdGroundShadowInputElementDesc, 1,
		mmd_ground_shadow_vso_data, sizeof(mmd_ground_shadow_vso_data),
		&m_mmdGroundShadowInputLayout
	);
	if (FAILED(hr))
	{
		return false;
	}

	// Blend State
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = device->CreateBlendState(&blendDesc, &m_mmdGroundShadowBlendState);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Rasterizer State
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = -1;
		rsDesc.SlopeScaledDepthBias = -1.0f;
		rsDesc.DepthBiasClamp = -1.0f;
		rsDesc.DepthClipEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		hr = device->CreateRasterizerState(&rsDesc, &m_mmdGroundShadowRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Depth Stencil State
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = 0x01;
		dsDesc.StencilWriteMask = 0xFF;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		hr = device->CreateDepthStencilState(&dsDesc, &m_mmdGroundShadowDSS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Default Depth Stencil State
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		dsDesc.StencilEnable = false;
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		hr = device->CreateDepthStencilState(&dsDesc, &m_defaultDSS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Dummy texture
	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = 1;
		tex2dDesc.Height = 1;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		hr = device->CreateTexture2D(&tex2dDesc, nullptr, &m_dummyTexture);
		if (FAILED(hr))
		{
			return false;
		}

		hr = device->CreateShaderResourceView(m_dummyTexture.Get(), nullptr, &m_dummyTextureView);
		if (FAILED(hr))
		{
			return false;
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = -FLT_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		hr = device->CreateSamplerState(&samplerDesc, &m_dummySampler);
		if (FAILED(hr))
		{
			return false;
		}
	}

	return true;
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
	if (mesh->IsPmx()) {
		// Draw model

		// Setup vertex shader
		{
			MMDVertexShaderCB vsCB;
			vsCB.m_wv = wv;
			vsCB.m_wvp = wvp;
			context->UpdateSubresource(m_mmdVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

			// Vertex shader
			context->VSSetShader(m_mmdVS.Get(), nullptr, 0);
			ID3D11Buffer* cbs[] = { m_mmdVSConstantBuffer.Get() };
			context->VSSetConstantBuffers(0, 1, cbs);
		}
		size_t subMeshCount = mesh->GetModel()->GetSubMeshCount();
		for (size_t i = 0; i < subMeshCount; i++)
		{
			const auto& subMesh = mesh->GetModel()->GetSubMeshes()[i];
			const auto& mat = m_materials[subMesh.m_materialID];
			const auto& mmdMat = mat.m_mmdMat;

			if (mat.m_mmdMat.m_alpha == 0)
			{
				continue;
			}

			// Pixel shader
			context->PSSetShader(appContext.m_mmdPS.Get(), nullptr, 0);

			MMDPixelShaderCB psCB;
			psCB.m_alpha = mmdMat.m_alpha;
			psCB.m_diffuse = mmdMat.m_diffuse;
			psCB.m_ambient = mmdMat.m_ambient;
			psCB.m_specular = mmdMat.m_specular;
			psCB.m_specularPower = mmdMat.m_specularPower;

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
				ID3D11ShaderResourceView* views[] = { mat.m_texture.m_textureView.Get() };
				ID3D11SamplerState* samplers[] = { appContext.m_textureSampler.Get() };
				context->PSSetShaderResources(0, 1, views);
				context->PSSetSamplers(0, 1, samplers);
			}
			else
			{
				psCB.m_textureModes.x = 0;
				ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
				ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
				context->PSSetShaderResources(0, 1, views);
				context->PSSetSamplers(0, 1, samplers);
			}

			if (mat.m_toonTexture.m_texture)
			{
				psCB.m_textureModes.y = 1;
				psCB.m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
				psCB.m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
				ID3D11ShaderResourceView* views[] = { mat.m_toonTexture.m_textureView.Get() };
				ID3D11SamplerState* samplers[] = { appContext.m_toonTextureSampler.Get() };
				context->PSSetShaderResources(1, 1, views);
				context->PSSetSamplers(1, 1, samplers);
			}
			else
			{
				psCB.m_textureModes.y = 0;
				ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
				ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
				context->PSSetShaderResources(1, 1, views);
				context->PSSetSamplers(1, 1, samplers);
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
				ID3D11ShaderResourceView* views[] = { mat.m_spTexture.m_textureView.Get() };
				ID3D11SamplerState* samplers[] = { appContext.m_sphereTextureSampler.Get() };
				context->PSSetShaderResources(2, 1, views);
				context->PSSetSamplers(2, 1, samplers);
			}
			else
			{
				psCB.m_textureModes.z = 0;
				ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
				ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
				context->PSSetShaderResources(2, 1, views);
				context->PSSetSamplers(2, 1, samplers);
			}

			psCB.m_lightColor = appContext.m_lightColor;
			glm::vec3 lightDir = appContext.m_lightDir;
			glm::mat3 viewMat = glm::mat3(appContext.m_viewMat);
			lightDir = viewMat * lightDir;
			psCB.m_lightDir = lightDir;

			context->UpdateSubresource(m_mmdPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);
			ID3D11Buffer* pscbs[] = { m_mmdPSConstantBuffer.Get() };
			context->PSSetConstantBuffers(1, 1, pscbs);

			if (mmdMat.m_bothFace)
			{
				context->RSSetState(appContext.m_mmdBothFaceRS.Get());
			}
			else
			{
				context->RSSetState(appContext.m_mmdFrontFaceRS.Get());
			}

			context->OMSetBlendState(appContext.m_mmdBlendState.Get(), nullptr, 0xffffffff);

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
			context->IASetInputLayout(appContext.m_mmdEdgeInputLayout.Get());
		}

		// Setup vertex shader (VSData)
		{
			MMDEdgeVertexShaderCB vsCB;
			vsCB.m_wv = wv;
			vsCB.m_wvp = wvp;
			vsCB.m_screenSize = glm::vec2(float(appContext.m_screenWidth), float(appContext.m_screenHeight));
			context->UpdateSubresource(m_mmdEdgeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

			// Vertex shader
			context->VSSetShader(appContext.m_mmdEdgeVS.Get(), nullptr, 0);
			ID3D11Buffer* cbs[] = { m_mmdEdgeVSConstantBuffer.Get() };
			context->VSSetConstantBuffers(0, 1, cbs);
		}

		for (size_t i = 0; i < subMeshCount; i++)
		{
			const auto& subMesh = mesh->GetModel()->GetSubMeshes()[i];
			const auto& mat = m_materials[subMesh.m_materialID];
			const auto& mmdMat = mat.m_mmdMat;

			if (!mmdMat.m_edgeFlag)
			{
				continue;
			}
			if (mmdMat.m_alpha == 0.0f)
			{
				continue;
			}

			// Edge size constant buffer
			{
				MMDEdgeSizeVertexShaderCB vsCB;
				vsCB.m_edgeSize = mmdMat.m_edgeSize;
				context->UpdateSubresource(m_mmdEdgeSizeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

				ID3D11Buffer* cbs[] = { m_mmdEdgeSizeVSConstantBuffer.Get() };
				context->VSSetConstantBuffers(1, 1, cbs);
			}

			// Pixel shader
			context->PSSetShader(appContext.m_mmdEdgePS.Get(), nullptr, 0);
			{
				MMDEdgePixelShaderCB psCB;
				psCB.m_edgeColor = mmdMat.m_edgeColor;
				context->UpdateSubresource(m_mmdEdgePSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

				ID3D11Buffer* pscbs[] = { m_mmdEdgePSConstantBuffer.Get() };
				context->PSSetConstantBuffers(2, 1, pscbs);
			}

			context->RSSetState(appContext.m_mmdEdgeRS.Get());

			context->OMSetBlendState(appContext.m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

			context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
		}
	}
	else {
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
	}

	if (m_drawDebugSphere && m_sphere) {
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
