#include "GameEntity.h"

#include "BufferStructs.h"
#include <WICTextureLoader.h>

#include <string>
#include <codecvt>
#include <locale>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//for saba shaders
#include "mmd.vso.h"
#include "mmd.pso.h"
#include "mmd_edge.vso.h"
#include "mmd_edge.pso.h"
#include "mmd_ground_shadow.vso.h"
#include "mmd_ground_shadow.pso.h"
#include <map>

#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

bool g_drawDebugSpheresDefault = true;
// mmd shader constant buffer

//========================
//Saba Code
//========================
#define	STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb/stb_image.h>

Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_mmdVS;
Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_mmdPS;
Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_mmdInputLayout;
Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_textureSampler;
Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_toonTextureSampler;
Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_sphereTextureSampler;
Microsoft::WRL::ComPtr<ID3D11BlendState>	m_mmdBlendState;
Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdFrontFaceRS;
Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdBothFaceRS;

Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_mmdEdgeVS;
Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_mmdEdgePS;
Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_mmdEdgeInputLayout;
Microsoft::WRL::ComPtr<ID3D11BlendState>	m_mmdEdgeBlendState;
Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdEdgeRS;

Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_mmdGroundShadowVS;
Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_mmdGroundShadowPS;
Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_mmdGroundShadowInputLayout;
Microsoft::WRL::ComPtr<ID3D11BlendState>	m_mmdGroundShadowBlendState;
Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdGroundShadowRS;
Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_mmdGroundShadowDSS;

Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_defaultDSS;

Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_dummyTexture;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_dummyTextureView;
Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_dummySampler;

Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
DXGI_FORMAT					m_indexBufferFormat;

Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdVSConstantBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdPSConstantBuffer;

Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdEdgeVSConstantBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdEdgeSizeVSConstantBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdEdgePSConstantBuffer;

Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdGroundShadowVSConstantBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer>		m_mmdGroundShadowPSConstantBuffer;

struct SabaVertex
{
	glm::vec3	m_position;
	glm::vec3	m_normal;
	glm::vec2	m_uv;
};

struct MMDVertexShaderCB
{
	DirectX::XMFLOAT4X4	m_wv;
	DirectX::XMFLOAT4X4 m_wvp;
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
	DirectX::XMFLOAT4X4	m_wv;
	DirectX::XMFLOAT4X4	m_wvp;
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
	DirectX::XMFLOAT4X4	m_wvp;
};	

struct MMDGroundShadowPixelShaderCB
{
	glm::vec4	m_shadowColor;
};

struct SabaMaterial
{
	explicit SabaMaterial(const saba::MMDMaterial& mat)
		: m_mmdMat(mat)
	{}

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const saba::MMDMaterial& m_mmdMat;
	GameEntity::Texture	m_texture;
	GameEntity::Texture	m_spTexture;
	GameEntity::Texture	m_toonTexture;
};

std::map<std::string, GameEntity::Texture>	m_textures;
std::vector<SabaMaterial>	m_materials;

GameEntity::Texture GameEntity::GetTexture(const std::string& texturePath)
{
	auto it = m_textures.find(texturePath);
	if (it == m_textures.end())
	{
		saba::File file;
		if (!file.Open(texturePath))
		{
			return Texture();
		}
		int x, y, comp;
		int ret = stbi_info_from_file(file.GetFilePointer(), &x, &y, &comp);
		if (ret == 0)
		{
			return Texture();
		}

		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = x;
		tex2dDesc.Height = y;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;

		int reqComp = 0;
		bool hasAlpha = false;
		if (comp != 4)
		{
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			hasAlpha = false;
		}
		else
		{
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			hasAlpha = true;
		}
		uint8_t* image = stbi_load_from_file(file.GetFilePointer(), &x, &y, &comp, STBI_rgb_alpha);
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = image;
		initData.SysMemPitch = 4 * x;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
		HRESULT hr = device->CreateTexture2D(&tex2dDesc, &initData, &tex2d);
		stbi_image_free(image);
		if (FAILED(hr))
		{
			return Texture();
		}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex2dRV;
		hr = device->CreateShaderResourceView(tex2d.Get(), nullptr, &tex2dRV);
		if (FAILED(hr))
		{
			return Texture();
		}

		Texture tex;
		tex.m_texture = tex2d;
		tex.m_textureView = tex2dRV;
		tex.m_hasAlpha = hasAlpha;

		m_textures[texturePath] = tex;

		return m_textures[texturePath];
	}
	else
	{
		return (*it).second;
	}
}

//========================
//End Saba Code
//========================


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
}

GameEntity::GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Camera> in_camera, Microsoft::WRL::ComPtr<ID3D11Device> in_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context,
	std::shared_ptr<SimpleVertexShader> vertexShader, std::shared_ptr<SimplePixelShader> pixelShader) {
	mesh = in_mesh;
	camera = in_camera;
	transform = Transform();
	device = in_device;
	context = in_context;

	m_rigidBody = std::make_shared<RigidBody>(&transform);
	m_collider = std::make_shared<Collider>(in_mesh, &transform);
	m_sphere = nullptr;
	m_drawDebugSphere = g_drawDebugSpheresDefault;
	m_isDebugSphere = false;

	if (mesh->IsPmx()) {
		SabaSetup();
		CreateSabaShaders();
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> one;//all white texture to represent ones
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> zero;//all black texture to represent zeros
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rampTexture;//all black texture to represent zeros
		DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
			GetFullPathTo_Wide(L"../../Assets/Textures/allMetal.png").c_str(), nullptr, one.GetAddressOf());
		DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
			GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, zero.GetAddressOf());
		DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
			GetFullPathTo_Wide(L"../../Assets/Textures/Ramp_Texture.png").c_str(), nullptr, rampTexture.GetAddressOf());


		//create description and sampler state
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // allows textures to tile
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; // allowing anisotropic filtering
		samplerDesc.MaxAnisotropy = 4;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //allways use mipmapping

		//create sampler
		device->CreateSamplerState(&samplerDesc, basicSampler.GetAddressOf());

		//create sampler state for post process
		D3D11_SAMPLER_DESC ppSamplerDesc = {};
		ppSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		ppSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		ppSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		ppSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; // allowing anisotropic filtering
		ppSamplerDesc.MaxAnisotropy = 4;
		ppSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //allways use mipmapping

		device->CreateSamplerState(&ppSamplerDesc, rampSampler.GetAddressOf());

		materials.reserve(mesh->GetModel()->GetMaterialCount());
		for (size_t i = 0; i < mesh->GetModel()->GetMaterialCount(); i++)
		{
			const auto& mmdTex = mesh->GetModel()->GetMaterials()[i].m_texture;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;

			//tutorial source https://riptutorial.com/cplusplus/example/4190/conversion-to-std--wstring
			std::wstring widePath = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(mmdTex);

			DirectX::CreateWICTextureFromFile(device.Get(), context.Get(),
				widePath.c_str(), nullptr, albedo.GetAddressOf());
			//Texture2D Tex : register(t0);
			//Texture2D ToonTex : register(t1);
			//Texture2D SphereTex : register(t2);
			//sampler TexSampler : register(s0);
			//sampler ToonTexSampler : register(s1);
			//sampler SphereTexSampler : register(s2);
			materials.push_back(std::make_shared<Material>(DirectX::XMFLOAT4(1.0, 1.0, 1.0f, 1.0f), 0.5f, vertexShader, pixelShader));
			materials[i]->AddSampler("TexSampler", m_textureSampler);
			materials[i]->AddSampler("ToonTexSampler", m_toonTextureSampler);
			materials[i]->AddSampler("SphereTexSampler", m_sphereTextureSampler);
			materials[i]->AddTextureSRV("Tex", albedo);
			materials[i]->AddTextureSRV("ToonTex", albedo);
			materials[i]->AddTextureSRV("SphereTex", albedo);
		}		
	}
}

bool GameEntity::SabaSetup() {
	auto t = sizeof(MMDPixelShaderCB);
	HRESULT hr;
	//hr = device->CreateDeferredContext(0, &context);
	//if (FAILED(hr))
	//{
	//	return false;
	//}

	// Setup vertex buffer
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.ByteWidth = UINT(sizeof(SabaVertex) * mesh->GetModel()->GetVertexCount());
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_vertexBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup index buffer;
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufDesc.ByteWidth = UINT(mesh->GetModel()->GetIndexElementSize() * mesh->GetModel()->GetIndexCount());
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = mesh->GetModel()->GetIndices();
		hr = device->CreateBuffer(&bufDesc, &initData, &m_indexBuffer);
		if (FAILED(hr))
		{
			return false;
		}

		if (1 == mesh->GetModel()->GetIndexElementSize())
		{
			m_indexBufferFormat = DXGI_FORMAT_R8_UINT;
		}
		else if (2 == mesh->GetModel()->GetIndexElementSize())
		{
			m_indexBufferFormat = DXGI_FORMAT_R16_UINT;
		}
		else if (4 == mesh->GetModel()->GetIndexElementSize())
		{
			m_indexBufferFormat = DXGI_FORMAT_R32_UINT;
		}
		else
		{
			return false;
		}
	}

	// Setup mmd vertex shader constant buffer (VSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd pixel shader constant buffer (PSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDPixelShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdPSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd edge vertex shader constant buffer (VSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDEdgeVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdEdgeVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd edge vertex shader constant buffer (VSEdgeData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDEdgeSizeVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdEdgeSizeVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd edge pixel shader constant buffer (PSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDEdgePixelShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdEdgePSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd ground shadow vertex shader constant buffer (VSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDGroundShadowVertexShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdGroundShadowVSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup mmd ground shadow pixel shader constant buffer (PSData)
	{
		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.ByteWidth = sizeof(MMDGroundShadowPixelShaderCB);
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bufDesc, nullptr, &m_mmdGroundShadowPSConstantBuffer);
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Setup materials
	std::shared_ptr<saba::PMXModel> m_mmdModel = mesh->GetModel();
	for (size_t i = 0; i < mesh->GetModel()->GetMaterialCount(); i++)
	{
		const auto& mmdMat = m_mmdModel->GetMaterials()[i];
		SabaMaterial mat(mmdMat);
		if (!mmdMat.m_texture.empty())
		{
			auto tex = GetTexture(mmdMat.m_texture);
			mat.m_texture = tex;
		}
		if (!mmdMat.m_spTexture.empty())
		{
			auto tex = GetTexture(mmdMat.m_spTexture);
			mat.m_spTexture = tex;
		}
		if (!mmdMat.m_toonTexture.empty())
		{
			auto tex = GetTexture(mmdMat.m_toonTexture);
			mat.m_toonTexture = tex;
		}
		m_materials.emplace_back(std::move(mat));
	}

	return true;
}

bool GameEntity::CreateSabaShaders() {
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

	// Setup materials
	for (size_t i = 0; i < mesh->GetModel()->GetMaterialCount(); i++)
	{
		const auto& mmdMat = mesh->GetModel()->GetMaterials()[i];
		SabaMaterial mat(mmdMat);
		if (!mmdMat.m_texture.empty())
		{
			auto tex = GetTexture(mmdMat.m_texture);
			mat.m_texture = tex;
		}
		if (!mmdMat.m_spTexture.empty())
		{
			auto tex = GetTexture(mmdMat.m_spTexture);
			mat.m_spTexture = tex;
		}
		if (!mmdMat.m_toonTexture.empty())
		{
			auto tex = GetTexture(mmdMat.m_toonTexture);
			mat.m_toonTexture = tex;
		}
		m_materials.emplace_back(std::move(mat));
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
		//m_sphere->GetTransform()->SetScale(1.5f, 1.5f, 1.5f);
		m_sphere->Draw();
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

	size_t subMeshCount = mesh->GetModel()->GetSubMeshCount();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = mesh->GetModel()->GetSubMeshes()[i];
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		materials[i]->PrepareMaterial();

		std::shared_ptr<SimpleVertexShader> vs = materials[i]->GetVertexShader();
		std::shared_ptr<SimplePixelShader> ps = materials[i]->GetPixelShader();

		materials[i]->GetVertexShader()->SetShader();
		materials[i]->GetPixelShader()->SetShader();
		//    float4x4 WV;
		//	  float4x4 WVP;

		//set the values for the vertex shader
		//string names MUST match those in VertexShader.hlsl
		vs->SetMatrix4x4("world", world);
		vs->SetMatrix4x4("WV", wv);
		vs->SetMatrix4x4("WVP", wvp);
		//set pixel shader buffer values
		//
		//float   Alpha;
		//float3  Diffuse;
		//float3  Ambient;
		//float3  Specular;
		//float   SpecularPower;
		//float3  LightColor;
		//float3  LightDir;
		//
		//float4  TexMulFactor;
		//float4  TexAddFactor;
		//
		//float4  ToonTexMulFactor;
		//float4  ToonTexAddFactor;
		//
		//float4  SphereTexMulFactor;
		//float4  SphereTexAddFactor;
		//
		//int4    TextureModes;

		int texMode[4] = {};

		if (mat.m_texture.m_texture)
		{
			if (!mat.m_texture.m_hasAlpha)
			{
				// Use Material Alpha
				texMode[0] = 1;
			}
			else
			{
				// Use Material Alpha * Texture Alpha
				texMode[0] = 2;
			}
		}
		else {
			texMode[0] = 0;
		}
		if (mat.m_toonTexture.m_texture)
		{
			texMode[1] = 1;
		}
		else {
			texMode[1] = 0;
		}
		if (mat.m_spTexture.m_texture)
		{
			if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
			{
				texMode[2] = 1;
			}
			else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
			{
				texMode[2] = 2;
			}
		}
		else {
			texMode[2] = 0;
		}

		ps->SetFloat("Alpha", mmdMat.m_alpha);
		ps->SetFloat3("Diffuse", DirectX::XMFLOAT3(mmdMat.m_diffuse[0], mmdMat.m_diffuse[1], mmdMat.m_diffuse[2]));
		ps->SetFloat3("Ambient", DirectX::XMFLOAT3(mmdMat.m_ambient[0], mmdMat.m_ambient[1], mmdMat.m_ambient[2]));
		ps->SetFloat3("Specular", DirectX::XMFLOAT3(mmdMat.m_specular[0], mmdMat.m_specular[1], mmdMat.m_specular[2]));
		ps->SetFloat("SpecularPower", mmdMat.m_specularPower);
		ps->SetFloat3("LightColor", m_lightColor);
		ps->SetFloat3("LightDir", m_lightDir);

		ps->SetFloat4("TexMulFactor", DirectX::XMFLOAT4(mmdMat.m_textureMulFactor[0], mmdMat.m_textureMulFactor[1], mmdMat.m_textureMulFactor[2], mmdMat.m_textureMulFactor[3]));
		ps->SetFloat4("TexAddFactor", DirectX::XMFLOAT4(mmdMat.m_textureAddFactor[0], mmdMat.m_textureAddFactor[1], mmdMat.m_textureAddFactor[2], mmdMat.m_textureAddFactor[3]));

		ps->SetFloat4("ToonTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureMulFactor[0], mmdMat.m_toonTextureMulFactor[1], mmdMat.m_toonTextureMulFactor[2], mmdMat.m_toonTextureMulFactor[3]));
		ps->SetFloat4("ToonTexAddFactor", DirectX::XMFLOAT4(mmdMat.m_toonTextureAddFactor[0], mmdMat.m_toonTextureAddFactor[1], mmdMat.m_toonTextureAddFactor[2], mmdMat.m_toonTextureAddFactor[3]));

		ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureMulFactor[0], mmdMat.m_spTextureMulFactor[1], mmdMat.m_spTextureMulFactor[2], mmdMat.m_spTextureMulFactor[3]));
		ps->SetFloat4("SphereTexMulFactor", DirectX::XMFLOAT4(mmdMat.m_spTextureAddFactor[0], mmdMat.m_spTextureAddFactor[1], mmdMat.m_spTextureAddFactor[2], mmdMat.m_spTextureAddFactor[3]));

		ps->SetData("TextureModes", texMode, sizeof(int) * 4);
		//copy data over to gpu. Equivelent to map, memcpy, unmap
		vs->CopyAllBufferData();
		ps->CopyAllBufferData();

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		//if (isPmx) {
		//	context->IASetIndexBuffer(indexBuf.Get(), format, 0);
		//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//}
		//else {
		//	context->IASetIndexBuffer(indexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
		//}
		context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetFormat(), 0);

		// Finally do the actual drawing
		// Once per object
		context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}
	/*

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
		vMat * wMat);
	DirectX::XMStoreFloat4x4(&wvp,
		pMat * vMat * wMat);

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
	//context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
	//	m_depthStencilView.Get()
	//);

	//context->OMSetDepthStencilState(m_defaultDSS.Get(), 0x00);



	size_t subMeshCount = mesh->GetModel()->GetSubMeshCount();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		// Setup input assembler
		//{
		//	UINT strides = sizeof(SabaVertex);
		//	UINT offsets = 0;
		context->IASetInputLayout(m_mmdInputLayout.Get());
		//	ID3D11Buffer* vbs[] = { m_vertexBuffer.Get() };
		//	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &strides, &offsets);
		//	context->IASetIndexBuffer(m_indexBuffer.Get(), m_indexBufferFormat, 0);
		//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//}


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

		const auto& subMesh = mesh->GetModel()->GetSubMeshes()[i];
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (mmdMat.m_alpha == 0)
		{
			continue;
		}

		//UINT stride = sizeof(Vertex);
		//UINT offset = 0;
		//context->IASetVertexBuffers(0, 1, mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		//if (isPmx) {
		//	context->IASetIndexBuffer(indexBuf.Get(), format, 0);
		//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//}
		//else {
		//	context->IASetIndexBuffer(indexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
		//}
		//context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetFormat(), 0);

		// Finally do the actual drawing
		// Once per object
		//context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);

		
		// Pixel shader
		context->PSSetShader(m_mmdPS.Get(), nullptr, 0);

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
			ID3D11SamplerState* samplers[] = { m_textureSampler.Get() };
			context->PSSetShaderResources(0, 1, views);
			context->PSSetSamplers(0, 1, samplers);
		}
		else
		{
			psCB.m_textureModes.x = 0;
			ID3D11ShaderResourceView* views[] = { m_dummyTextureView.Get() };
			ID3D11SamplerState* samplers[] = { m_dummySampler.Get() };
			context->PSSetShaderResources(0, 1, views);
			context->PSSetSamplers(0, 1, samplers);
		}

		if (mat.m_toonTexture.m_texture)
		{
			psCB.m_textureModes.y = 1;
			psCB.m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
			psCB.m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
			ID3D11ShaderResourceView* views[] = { mat.m_toonTexture.m_textureView.Get() };
			ID3D11SamplerState* samplers[] = { m_toonTextureSampler.Get() };
			context->PSSetShaderResources(1, 1, views);
			context->PSSetSamplers(1, 1, samplers);
		}
		else
		{
			psCB.m_textureModes.y = 0;
			ID3D11ShaderResourceView* views[] = { m_dummyTextureView.Get() };
			ID3D11SamplerState* samplers[] = { m_dummySampler.Get() };
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
			ID3D11SamplerState* samplers[] = { m_sphereTextureSampler.Get() };
			context->PSSetShaderResources(2, 1, views);
			context->PSSetSamplers(2, 1, samplers);
		}
		else
		{
			psCB.m_textureModes.z = 0;
			ID3D11ShaderResourceView* views[] = { m_dummyTextureView.Get() };
			ID3D11SamplerState* samplers[] = { m_dummySampler.Get() };
			context->PSSetShaderResources(2, 1, views);
			context->PSSetSamplers(2, 1, samplers);
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

		context->UpdateSubresource(m_mmdPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);
		ID3D11Buffer* pscbs[] = { m_mmdPSConstantBuffer.Get() };
		context->PSSetConstantBuffers(1, 1, pscbs);

		if (mmdMat.m_bothFace)
		{
			context->RSSetState(m_mmdBothFaceRS.Get());
		}
		else
		{
			context->RSSetState(m_mmdFrontFaceRS.Get());
		}

		//context->OMSetBlendState(m_mmdBlendState.Get(), nullptr, 0xffffffff);

		//context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}
	/*
	{
		ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr };
		ID3D11SamplerState* samplers[] = { nullptr, nullptr, nullptr };
		context->PSSetShaderResources(0, 3, views);
		context->PSSetSamplers(0, 3, samplers);
	}

	// Draw edge

	// Setup input assembler
	{
		context->IASetInputLayout(m_mmdEdgeInputLayout.Get());
	}

	// Setup vertex shader (VSData)
	{
		MMDEdgeVertexShaderCB vsCB;
		vsCB.m_wv = wv;
		vsCB.m_wvp = wvp;
		vsCB.m_screenSize = glm::vec2(float(m_screenWidth), float(m_screenHeight));
		context->UpdateSubresource(m_mmdEdgeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

		// Vertex shader
		context->VSSetShader(m_mmdEdgeVS.Get(), nullptr, 0);
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
		context->PSSetShader(m_mmdEdgePS.Get(), nullptr, 0);
		{
			MMDEdgePixelShaderCB psCB;
			psCB.m_edgeColor = mmdMat.m_edgeColor;
			context->UpdateSubresource(m_mmdEdgePSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

			ID3D11Buffer* pscbs[] = { m_mmdEdgePSConstantBuffer.Get() };
			context->PSSetConstantBuffers(2, 1, pscbs);
		}

		context->RSSetState(m_mmdEdgeRS.Get());

		context->OMSetBlendState(m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

		context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	}*/

	//end saba lib example
	//}
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
