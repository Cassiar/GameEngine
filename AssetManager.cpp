#include "AssetManager.h"

using namespace DirectX;

std::shared_ptr<AssetManager> AssetManager::s_instance;

AssetManager::AssetManager(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	Init(device, context);
}
//Nothing to delete right now
AssetManager::~AssetManager() {}

void AssetManager::InitializeSingleton(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	if (!s_instance.get()) {
		std::shared_ptr<AssetManager> newInstance(new AssetManager(device, context));
		s_instance = newInstance;
	}
}

std::shared_ptr<AssetManager> AssetManager::GetInstance()
{
	return s_instance;
}

void AssetManager::Init(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	m_device = device;
	m_context = context;

	// Some of the basic map lists that we already need
	m_srvMaps = std::unordered_map<SRVMaps, std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>>();

	m_srvMaps[SRVMaps::Albedo] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Roughness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::AO] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Normal] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::Metalness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();

	m_srvMaps[SRVMaps::ToonAlbedo] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonRoughness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonAO] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
	m_srvMaps[SRVMaps::ToonMetalness] = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();

	m_vertexShaders = std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>>();
	m_pixelShaders = std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>>();

	m_meshes = std::vector<std::shared_ptr<Mesh>>();
	m_toonMeshes = std::vector<std::shared_ptr<Mesh>>();
	m_sabaMeshes = std::vector<std::shared_ptr<SabaMesh>>();

	InitTextures();
	InitShaders();
	InitMeshes();
	InitSamplers();
	InitMaterials();

	InitSaba(m_sabaMeshes[0]);
	InitSabaShaders(m_sabaMeshes[0]);
	InitSabaMaterials(m_sabaMeshes[0]);
}

void AssetManager::InitTextures()
{
	AddSRVToMap(Albedo,		L"Medieval_Floor_Albedo.tif");
	AddSRVToMap(Roughness,	L"Medieval_Floor_Roughness.tif");
	AddSRVToMap(AO,			L"Medieval_Floor_AO.tif");
	AddSRVToMap(Normal,		L"Medieval_Floor_Normal.tif");
	AddSRVToMap(Metalness,	L"noMetal.png");

	AddSRVToMap(Albedo,		L"SciFi_Panel_Albedo.tif");
	AddSRVToMap(Roughness,	L"SciFi_Panel_Roughness.tif");
	AddSRVToMap(AO,			L"SciFi_Panel_AO.tif");
	AddSRVToMap(Normal,		L"SciFi_Panel_Normal.tif");
	AddSRVToMap(Metalness,	L"SciFi_Panel_Metalness.tif");

	AddSRVToMap(Albedo,		L"Brick_Wall_Albedo.tif");
	AddSRVToMap(Roughness,	L"Brick_Wall_Roughness.tif");
	AddSRVToMap(AO,			L"Brick_Wall_AO.tif");
	AddSRVToMap(Normal,		L"Brick_Wall_Normal.tif");
	AddSRVToMap(Metalness,	L"noMetal.png");

	AddSRVToMap(Albedo,		L"Bronze_Albedo.tif");
	AddSRVToMap(Roughness,	L"Bronze_Roughness.tif");
	AddSRVToMap(AO,			L"Bronze_AO.tif");
	AddSRVToMap(Normal,		L"Bronze_Normal.tif");
	AddSRVToMap(Metalness,	L"Bronze_Metallic.tif");

	AddSRVToMap(ToonAlbedo,		L"Tree_Albedo.tif");
	AddSRVToMap(ToonRoughness,	L"noMetal.png");
	AddSRVToMap(ToonAO,			L"allMetal.png");
	AddSRVToMap(ToonMetalness,	L"noMetal.png");

	AddSRVToMap(SampleTexture, L"Ramp_Texture.png");

	//load cube map
	m_srvMaps[SkyBox].push_back(CreateCubemap(
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_right.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_left.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_up.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_down.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_front.png").c_str(),
							GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_back.png").c_str()));
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void AssetManager::InitShaders()
{
	//using Chris's simple shader
	AddVertShaderToMap("vertexShader",				"VertexShader.cso");
	AddVertShaderToMap("skyVertexShader",			"SkyVertexShader.cso");
	AddVertShaderToMap("shadowVertexShader",		"ShadowVertexShader.cso");
	AddVertShaderToMap("ppLightRaysVertexShader",	"PostProcessLightRaysVertexShader.cso");
	//AddVertShaderToMap("mmdVertexShader",			"MMDVertexShader.cso");
	//AddVertShaderToMap("mmdEdgeVertexShader",		"MMDEdgeVertexShader.cso");

	AddPixelShaderToMap("pixelShader",				"PixelShader.cso");
	AddPixelShaderToMap("debugPixelShader",			"DebugColorShader.cso");
	AddPixelShaderToMap("skyPixelShader",			"SkyPixelShader.cso");
	AddPixelShaderToMap("shadowPixelShader",		"ShadowPixelShader.cso");
	AddPixelShaderToMap("toonPixelShader",			"ToonPixelShader.cso");
	AddPixelShaderToMap("ppLightRaysPixelShader",	"PostProcessLightRaysPixelShader.cso");
	AddPixelShaderToMap("a5PixelShader",			"A5CustomPS.cso");
	//AddPixelShaderToMap("mmdPixelShader",			"MMDPixelShader.cso");
	//AddPixelShaderToMap("mmdEdgePixelShader",		"MMDEdgePixelShader.cso");
}

void AssetManager::InitMeshes()
{
	m_meshes.push_back(LoadMesh("cube.obj"));
	m_meshes.push_back(LoadMesh("cylinder.obj"));
	m_meshes.push_back(LoadMesh("helix.obj"));
	m_meshes.push_back(LoadMesh("sphere.obj"));
	m_meshes.push_back(LoadMesh("quad.obj"));

	m_toonMeshes.push_back(LoadMesh("Tree.obj"));

	m_sabaMeshes.push_back(std::make_shared<SabaMesh>(GetFullPathTo("../../Assets/Toon/Lisa/Lisa_Textured.pmx").c_str(), GetFullPathTo("../../Assets/Toon/Lisa/Texture").c_str(), m_device, m_context));
}

void AssetManager::InitSamplers()
{
	//create description and sampler state
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // allows textures to tile
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; // allowing anisotropic filtering
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //allways use mipmapping

	m_samplers["basicSampler"] = Microsoft::WRL::ComPtr<ID3D11SamplerState>();
	//create sampler
	m_device->CreateSamplerState(&samplerDesc, m_samplers["basicSampler"].GetAddressOf());


	//create sampler state for post process
	D3D11_SAMPLER_DESC ppSamplerDesc = {};
	ppSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; // allowing anisotropic filtering
	ppSamplerDesc.MaxAnisotropy = 4;
	ppSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //allways use mipmapping

	m_samplers["ppLightRaysSampler"] = Microsoft::WRL::ComPtr<ID3D11SamplerState>();
	m_device->CreateSamplerState(&ppSamplerDesc, m_samplers["ppLightRaysSampler"].GetAddressOf());

	//create sampler state for post process
	D3D11_SAMPLER_DESC rampSamplerDesc = {};
	rampSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	rampSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	rampSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	rampSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; // allowing anisotropic filtering
	rampSamplerDesc.MaxAnisotropy = 4;
	rampSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //allways use mipmapping

	m_samplers["rampSampler"] = Microsoft::WRL::ComPtr<ID3D11SamplerState>();
	m_device->CreateSamplerState(&ppSamplerDesc, m_samplers["rampSampler"].GetAddressOf());

	//create description and sampler state
	D3D11_SAMPLER_DESC sabaSamplerDesc = {};
	sabaSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sabaSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sabaSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // allows textures to tile
	sabaSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; // allowing anisotropic filtering
	sabaSamplerDesc.MaxAnisotropy = 4;
	sabaSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //allways use mipmapping

	m_samplers["sabaSampler"] = Microsoft::WRL::ComPtr<ID3D11SamplerState>();
	m_device->CreateSamplerState(&sabaSamplerDesc, m_samplers["sabaSampler"].GetAddressOf());
}

void AssetManager::InitMaterials()
{
	//create the materials

	for (int i = 1; i < 5; i++)
	{
		std::shared_ptr<Material> temp = std::make_shared<Material>(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, nullptr, "", "", nullptr, "", "");
		MaterialSerialData thisMat;
		std::wstring path(L"Assets/Materials/Mat" + std::to_wstring(i) + L".nsm");
		temp->ReadBinary(path, thisMat);
		MakeMaterialFromSerial(thisMat, temp);
		m_materials.push_back(temp);
	}
	//m_materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, m_vertexShaders["vertexShader"], m_vertShaderCsoNames["vertexShader"], "vertexShader", m_pixelShaders["pixelShader"], m_pixelShaderCsoNames["pixelShader"], "pixelShader"));//white material for medieval floor
	//m_materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, m_vertexShaders["vertexShader"], m_vertShaderCsoNames["vertexShader"], "vertexShader", m_pixelShaders["pixelShader"], m_pixelShaderCsoNames["pixelShader"], "pixelShader"));//white material for scifi panel
	//m_materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, m_vertexShaders["vertexShader"], m_vertShaderCsoNames["vertexShader"], "vertexShader", m_pixelShaders["pixelShader"], m_pixelShaderCsoNames["pixelShader"], "pixelShader"));//white material for cobblestone wall
	//m_materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, m_vertexShaders["vertexShader"], m_vertShaderCsoNames["vertexShader"], "vertexShader", m_pixelShaders["pixelShader"], m_pixelShaderCsoNames["pixelShader"], "pixelShader"));//white material for bronze
	//materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, toonPixelShader)); //toon shader material for testing
	//catapultMaterial = std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, catapultPixelShader);

	//add textures to each material
	for (unsigned int i = 0; i < m_materials.size(); i++) {
		m_materials[i]->AddSampler("BasicSampler",			m_samplers["basicSampler"]);
		m_materials[i]->AddTextureSRV("AlbedoTexture",		m_srvMaps[Albedo][i],		m_srvFileNames[Albedo][i], Albedo);
		m_materials[i]->AddTextureSRV("RoughnessTexture",	m_srvMaps[Roughness][i],	m_srvFileNames[Roughness][i], Roughness);
		m_materials[i]->AddTextureSRV("AmbientTexture",		m_srvMaps[AO][i],			m_srvFileNames[AO][i], AO);
		m_materials[i]->AddTextureSRV("NormalTexture",		m_srvMaps[Normal][i],		m_srvFileNames[Normal][i], Normal);
		m_materials[i]->AddTextureSRV("MetalnessTexture",	m_srvMaps[Metalness][i],	m_srvFileNames[Metalness][i], Metalness);
	}

	//toon shader. for testing uses scifi panel
	m_toonMaterials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, m_vertexShaders["vertexShader"], m_vertShaderCsoNames["vertexShader"], "vertexShader", m_pixelShaders["toonPixelShader"], m_pixelShaderCsoNames["toonPixelShader"], "toonPixelShader"));

	m_toonMaterials[0]->AddSampler("BasicSampler",			m_samplers["basicSampler"]);
	m_toonMaterials[0]->AddSampler("RampSampler",			m_samplers["ppLightRaysSampler"]);
	m_toonMaterials[0]->AddTextureSRV("AlbedoTexture",		m_srvMaps[ToonAlbedo][0],		m_srvFileNames[ToonAlbedo][0], ToonAlbedo);
	m_toonMaterials[0]->AddTextureSRV("RoughnessTexture",	m_srvMaps[ToonRoughness][0],	m_srvFileNames[ToonRoughness][0], ToonRoughness);
	m_toonMaterials[0]->AddTextureSRV("AmbientTexture",		m_srvMaps[ToonAO][0],			m_srvFileNames[ToonAO][0], ToonAO);
	m_toonMaterials[0]->AddTextureSRV("RampTexture",		m_srvMaps[SampleTexture][0],	m_srvFileNames[SampleTexture][0], SampleTexture);
	m_toonMaterials[0]->AddTextureSRV("MetalnessTexture",	m_srvMaps[ToonMetalness][0],	m_srvFileNames[ToonMetalness][0], ToonMetalness);
}


bool AssetManager::InitSaba(std::shared_ptr<SabaMesh> mesh) {
	auto t = sizeof(MMDPixelShaderCB);

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
		m_sabaStructMaterials.emplace_back(std::move(mat));
	}

	return true;
}

bool AssetManager::InitSabaShaders(std::shared_ptr<SabaMesh> mesh) {
	HRESULT hr;

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
		hr = m_device->CreateSamplerState(&samplerDesc, &m_textureSampler);
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
		hr = m_device->CreateSamplerState(&samplerDesc, &m_toonTextureSampler);
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
		hr = m_device->CreateSamplerState(&samplerDesc, &m_sphereTextureSampler);
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
		hr = m_device->CreateBlendState(&blendDesc, &m_mmdBlendState);
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
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdFrontFaceRS);
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
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdBothFaceRS);
		if (FAILED(hr))
		{
			return false;
		}
	}

	D3D11_INPUT_ELEMENT_DESC mmdEdgeInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = m_device->CreateInputLayout(
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
		hr = m_device->CreateBlendState(&blendDesc, &m_mmdEdgeBlendState);
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
		hr = m_device->CreateRasterizerState(&rsDesc, &m_mmdEdgeRS);
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
		hr = m_device->CreateDepthStencilState(&dsDesc, &m_defaultDSS);
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
		hr = m_device->CreateTexture2D(&tex2dDesc, nullptr, &m_dummyTexture);
		if (FAILED(hr))
		{
			return false;
		}

		hr = m_device->CreateShaderResourceView(m_dummyTexture.Get(), nullptr, &m_dummyTextureView);
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
		hr = m_device->CreateSamplerState(&samplerDesc, &m_dummySampler);
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
		m_sabaStructMaterials.emplace_back(std::move(mat));
	}

	return true;
}

void AssetManager::InitSabaMaterials(std::shared_ptr<SabaMesh> mesh) {
	m_sabaMaterials.reserve(mesh->GetModel()->GetMaterialCount());
	m_vertexShaders["edgeVertexShader"] = std::make_shared<SimpleVertexShader>(m_device, m_context, GetFullPathTo_Wide(L"MMDEdgeVertexShader.cso").c_str());
	m_pixelShaders["edgePixelShader"] = std::make_shared<SimplePixelShader>(m_device, m_context, GetFullPathTo_Wide(L"MMDEdgePixelShader.cso").c_str());
	
	m_vertexShaders["mmdVertexShader"] = std::make_shared<SimpleVertexShader>(m_device, m_context, GetFullPathTo_Wide(L"MMDVertexShader.cso").c_str());
	m_pixelShaders["mmdPixelShader"] = std::make_shared<SimplePixelShader>(m_device, m_context, GetFullPathTo_Wide(L"MMDPixelShader.cso").c_str());
	for (size_t i = 0; i < mesh->GetModel()->GetMaterialCount(); i++)
	{
		const auto& mmdTex = mesh->GetModel()->GetMaterials()[i].m_texture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;

		std::wstring widePath = StringToWide(mmdTex);

		DirectX::CreateWICTextureFromFile(m_device.Get(), m_context.Get(),
			widePath.c_str(), nullptr, albedo.GetAddressOf());

		m_sabaMaterials.push_back(std::make_shared<Material>(DirectX::XMFLOAT4(1.0, 1.0, 1.0f, 1.0f), 0.5f, m_vertexShaders["mmdVertexShader"], m_vertShaderCsoNames["mmdVertexShader"], "mmdVertexShader", m_pixelShaders["mmdPixelShader"], m_pixelShaderCsoNames["mmdPixelShader"], "mmdPixelShader"));
		m_sabaMaterials[i]->AddSampler("TexSampler", m_textureSampler);
		m_sabaMaterials[i]->AddSampler("ToonTexSampler", m_toonTextureSampler);
		m_sabaMaterials[i]->AddSampler("SphereTexSampler", m_sphereTextureSampler);
		m_sabaMaterials[i]->AddTextureSRV("Tex", albedo, WideToString(widePath), Albedo);
		m_sabaMaterials[i]->AddTextureSRV("ToonTex", albedo,WideToString(widePath), ToonAlbedo);
		m_sabaMaterials[i]->AddTextureSRV("SphereTex", albedo, WideToString(widePath), SampleTexture);

		m_sabaEdgeMaterials.push_back(std::make_shared<Material>(DirectX::XMFLOAT4(1.0, 1.0, 1.0f, 1.0f), 0.5f, m_vertexShaders["edgeVertexShader"], m_vertShaderCsoNames["edgeVertexShader"], "edgeVertexShader", m_pixelShaders["edgePixelShader"], m_pixelShaderCsoNames["edgePixelShader"], "mmdPixelShader"));
	}
}


SabaTexture AssetManager::GetTexture(const std::string& texturePath)
{
	auto it = m_sabaTextures.find(texturePath);
	if (it == m_sabaTextures.end())
	{
		saba::File file;
		if (!file.Open(texturePath))
		{
			return SabaTexture();
		}
		int x, y, comp;
		int ret = stbi_info_from_file(file.GetFilePointer(), &x, &y, &comp);
		if (ret == 0)
		{
			return SabaTexture();
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
		HRESULT hr = m_device->CreateTexture2D(&tex2dDesc, &initData, &tex2d);
		stbi_image_free(image);
		if (FAILED(hr))
		{
			return SabaTexture();
		}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex2dRV;
		hr = m_device->CreateShaderResourceView(tex2d.Get(), nullptr, &tex2dRV);
		if (FAILED(hr))
		{
			return SabaTexture();
		}

		SabaTexture tex;
		tex.m_texture = tex2d;
		tex.m_textureView = tex2dRV;
		tex.m_hasAlpha = hasAlpha;

		m_sabaTextures[texturePath] = tex;

		return m_sabaTextures[texturePath];
	}
	else
	{
		return (*it).second;
	}
}

std::vector<std::string> AssetManager::GetSamplerNames()
{
	std::vector<std::string> names = std::vector<std::string>();
	for (const auto& kv : m_samplers)
		names.push_back(kv.first);

	return names;
}

//helper function to create cube map from 6 textures.
//used with permission from Chris Cascioli
// --------------------------------------------------------
// Loads six individual textures (the six faces of a cube map), then
// creates a blank cube map and copies each of the six textures to
// another face. Afterwards, creates a shader resource view for
// the cube map and cleans up all of the temporary resources.
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::CreateCubemap(
	const wchar_t* right,
	const wchar_t* left,
	const wchar_t* up,
	const wchar_t* down,
	const wchar_t* front,
	const wchar_t* back)
{
	// Load the 6 textures into an array.
	// - We need references to the TEXTURES, not the SHADER RESOURCE VIEWS!
	// - Specifically NOT generating mipmaps, as we usually don't need them for the sky!
	// - Order matters here! +X, -X, +Y, -Y, +Z, -Z
	ID3D11Texture2D* textures[6] = {};
	CreateWICTextureFromFile(m_device.Get(), right, (ID3D11Resource**)&textures[0], 0);
	CreateWICTextureFromFile(m_device.Get(), left, (ID3D11Resource**)&textures[1], 0);
	CreateWICTextureFromFile(m_device.Get(), up, (ID3D11Resource**)&textures[2], 0);
	CreateWICTextureFromFile(m_device.Get(), down, (ID3D11Resource**)&textures[3], 0);
	CreateWICTextureFromFile(m_device.Get(), front, (ID3D11Resource**)&textures[4], 0);
	CreateWICTextureFromFile(m_device.Get(), back, (ID3D11Resource**)&textures[5], 0);

	// We'll assume all of the textures are the same color format and resolution,
	// so get the description of the first shader resource view
	D3D11_TEXTURE2D_DESC faceDesc = {};
	textures[0]->GetDesc(&faceDesc);

	// Describe the resource for the cube map, which is simply
	// a "texture 2d array". This is a special GPU resource format,
	// NOT just a C++ array of textures!!!
	D3D11_TEXTURE2D_DESC cubeDesc = {};
	cubeDesc.ArraySize = 6; // Cube map!
	cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // We'll be using as a texture in a shader
	cubeDesc.CPUAccessFlags = 0; // No read back
	cubeDesc.Format = faceDesc.Format; // Match the loaded texture's color format
	cubeDesc.Width = faceDesc.Width; // Match the size
	cubeDesc.Height = faceDesc.Height; // Match the size
	cubeDesc.MipLevels = 1; // Only need 1
	cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // A CUBE MAP, not 6 separate textures
	cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage
	cubeDesc.SampleDesc.Count = 1;
	cubeDesc.SampleDesc.Quality = 0;

	// Create the actual texture resource
	ID3D11Texture2D* cubeMapTexture = 0;
	m_device->CreateTexture2D(&cubeDesc, 0, &cubeMapTexture);

	// Loop through the individual face textures and copy them,
	// one at a time, to the cube map texure
	for (int i = 0; i < 6; i++)
	{
		// Calculate the subresource position to copy into
		unsigned int subresource = D3D11CalcSubresource(
			0, // Which mip (zero, since there's only one)
			i, // Which array element?
			1); // How many mip levels are in the texture?

		// Copy from one resource (texture) to another
		if (cubeMapTexture && textures[i]) {
			m_context->CopySubresourceRegion(
				cubeMapTexture, // Destination resource
				subresource, // Dest subresource index (one of the array elements)
				0, 0, 0, // XYZ location of copy
				textures[i], // Source resource
				0, // Source subresource index (we're assuming there's only one)
				0); // Source subresource "box" of data to copy (zero means the whole thing)
		}
	}

	// At this point, all of the faces have been copied into the
	// cube map texture, so we can describe a shader resource view for it
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = cubeDesc.Format; // Same format as texture
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
	srvDesc.TextureCube.MipLevels = 1; // Only need access to 1 mip
	srvDesc.TextureCube.MostDetailedMip = 0; // Index of the first mip we want to see

	// Make the SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
	if (cubeMapTexture) {
		m_device->CreateShaderResourceView(cubeMapTexture, &srvDesc, cubeSRV.GetAddressOf());
	}

	// Now that we're done, clean up the stuff we don't need anymore
	cubeMapTexture->Release(); // Done with this particular reference (the SRV has another)
	for (int i = 0; i < 6; i++)
		textures[i]->Release();
	// Send back the SRV, which is what we need for our shaders
	return cubeSRV;
}



Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadSRV(std::wstring texturePath, bool customLocation /*Default = false*/)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	texture = LoadSRV(texturePath, texture, customLocation);

	return texture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadSRV(std::wstring texturePath, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureDestination, bool customLocation /*Default = false*/)
{
	std::wstring path = (customLocation ? L"" : TEXTURE_FOLDER);
	path += texturePath;

	CreateWICTextureFromFile(m_device.Get(), m_context.Get(),
		GetFullPathTo_Wide(path).c_str(), nullptr, textureDestination.GetAddressOf());

	return textureDestination;
}

std::shared_ptr<Mesh> AssetManager::LoadMesh(std::string meshPath, bool customLocation /*Default = false*/)
{
	std::string path = (customLocation ? "" : MODEL_FOLDER);
	path += meshPath;
	return std::make_shared<Mesh>(GetFullPathTo(path).c_str(), m_device, m_context);
}

//void AssetManager::AddSRVToMap(SRVMaps mapTypeName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvToAdd)
//{//Deprecated func should be removed outside of this case
//	if (mapTypeName != SkyBox)
//	{
//		return;
//	}
//	m_srvMaps[mapTypeName].push_back(srvToAdd);
//}

void AssetManager::AddSRVToMap(SRVMaps mapTypeName, std::wstring srvPath, bool customPath /* Default = false */)
{
	m_srvMaps[mapTypeName].push_back(LoadSRV(srvPath));
	m_srvFileNames[mapTypeName].push_back(WideToString(srvPath));
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::GetSRV(SRVMaps map, int srvIndex)
{
	return m_srvMaps[map][srvIndex];
}

void AssetManager::AddVertShaderToMap(std::string key, std::string filename)
{
	m_vertexShaders[key] = MakeSimpleVertexShader(StringToWide(filename));
	m_vertShaderCsoNames[key] = filename;
}

std::shared_ptr<SimpleVertexShader> AssetManager::MakeSimpleVertexShader(std::wstring csoName)
{
	return std::make_shared<SimpleVertexShader>(m_device, m_context, GetFullPathTo_Wide(csoName).c_str());
}

void AssetManager::AddPixelShaderToMap(std::string key, std::string filename)
{
	m_pixelShaders[key] = MakeSimplePixelShader(StringToWide(filename));
	m_pixelShaderCsoNames[key] = filename;
}

std::shared_ptr<SimplePixelShader> AssetManager::MakeSimplePixelShader(std::wstring csoName)
{
	return std::make_shared<SimplePixelShader>(m_device, m_context, GetFullPathTo_Wide(csoName).c_str());
}

void AssetManager::MakeRasterizerState(D3D11_RASTERIZER_DESC rastDesc, Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rastLocation) {
	m_device->CreateRasterizerState(&rastDesc, rastLocation.GetAddressOf());
}


std::shared_ptr<Material> AssetManager::ReadMaterialFromFile(std::wstring path) {
	std::ifstream rStream(path, std::ios::in | std::ios::binary);
	if (!rStream) {
		//std::cout << "Cannot open file!" << std::endl;
	}
	std::vector<TypeKey> types = std::vector<TypeKey>();
	char* readC = new char[1024];
	rStream.read(readC, sizeof(char));

	int readCount = readC[0] - '0';
	for (int i = 0; i < readCount; i++)
	{
		rStream.read(readC, sizeof(char));
		types.push_back(static_cast<TypeKey>(readC[0] - '0'));
	}


	rStream.close();
	if (!rStream.good()) {
		//std::cout << "Error occurred at reading time!" << std::endl;
	}

	//std::cout << temp << " success!" << std::endl;
	delete[] readC;

	return m_materials[0];
}

void AssetManager::MakeMaterialFromSerial(MaterialSerialData data, std::shared_ptr<Material> writeMaterial) {
	writeMaterial->SetColorTint(data.colorTint);
	writeMaterial->SetRoughness(data.roughness);

	for (int i = 0; i < MATERIAL_MAX_SERIAL_SRVS; i++)
	{
		if (data.srvFileNames[i] == "")
			continue;

		SRVMaps type = data.srvTypes[i];
		AddSRVToMap(type, StringToWide(data.srvFileNames[i]), true);
		int lastIndex = m_srvMaps[type].size() - 1;
		writeMaterial->AddTextureSRV(data.srvNames[i], m_srvMaps[type][lastIndex], data.srvFileNames[i], data.srvTypes[i]);
	}

	if (m_vertexShaders[data.vsName] == nullptr)
	{
		AddVertShaderToMap(data.vsName, data.vsFileName);
	}
	writeMaterial->SetVertexShader(m_vertexShaders[data.vsName], data.vsFileName, data.vsName);
	
	if (m_pixelShaders[data.psName] == nullptr)
	{
		AddPixelShaderToMap(data.psName, data.psFileName);
	}
	writeMaterial->SetPixelShader(m_pixelShaders[data.psName], data.psFileName, data.psName);

	for (int i = 0; i < MATERIAL_MAX_SERIAL_SAMPLERS; i++) {
		if (m_samplers[data.samplerNames[i]] == nullptr)
			continue;

		writeMaterial->AddSampler(data.samplerNames[i], m_samplers[data.samplerNames[i]]);
	}
}

///------------------ Written by Chris Cascioli ------------------------------///

// --------------------------------------------------------------------------
// Gets the actual path to this executable
//
// - As it turns out, the relative path for a program is different when 
//    running through VS and when running the .exe directly, which makes 
//    it a pain to properly load external files (like textures)
//    - Running through VS: Current Dir is the *project folder*
//    - Running from .exe:  Current Dir is the .exe's folder
// - This has nothing to do with DEBUG and RELEASE modes - it's purely a 
//    Visual Studio "thing", and isn't obvious unless you know to look 
//    for it.  In fact, it could be fixed by changing a setting in VS, but
//    the option is stored in a user file (.suo), which is ignored by most
//    version control packages by default.  Meaning: the option must be
//    changed on every PC.  Ugh.  So instead, here's a helper.
// --------------------------------------------------------------------------
std::string AssetManager::GetExePath()
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
std::wstring AssetManager::GetExePath_Wide()
{
	// Grab the path as a standard string
	std::string path = GetExePath();

	// Convert to a wide string
	wchar_t widePath[1024] = {};
	mbstowcs_s(0, widePath, path.c_str(), 1024);

	// Create a wstring for it and return
	return std::wstring(widePath);
}

std::wstring AssetManager::StringToWide(std::string str) {
	wchar_t wide[1024] = {};
	mbstowcs_s(0, wide, str.c_str(), 1024);

	// Create a wstring for it and return
	return std::wstring(wide);
}

std::string AssetManager::WideToString(std::wstring str) {
	char reg[1024] = {};
	wcstombs_s(0, reg, str.c_str(), 1024);

	// Create a wstring for it and return
	return std::string(reg);
}


// ----------------------------------------------------
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::string AssetManager::GetFullPathTo(std::string relativeFilePath)
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
std::wstring AssetManager::GetFullPathTo_Wide(std::wstring relativeFilePath)
{
	return GetExePath_Wide() + L"\\" + relativeFilePath;
}

///---------------------------------------------------------------------------///