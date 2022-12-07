#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "BufferStructs.h"

#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
//#include "imgui_draw.cpp"

#include "WICTextureLoader.h" //loading textures, in DirectX namespace

#include <Saba/Base/Time.h>

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;
using namespace physx;

float Collider::m_debugSphereMeshRadius;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true),			   // Show extra stats (fps) in title bar?
	vsync(true)			//should we lock framerate
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
	printf("%i, %i", this->width, this->height);
	camera = std::make_shared<Camera>(XMFLOAT3(0.0f, 6.5f, -50), (float)this->width / this->height, XM_PIDIV4, 0.1f, 1000.0f);
	camera->GetTransform()->Rotate(XMFLOAT3(0, 0, 0));
	camera->UpdateViewMatrix();
	camera->GetViewMatrix();
	ambientTerm = XMFLOAT3(0.0f, 0.0f, 0.0f); //sky red-ish to match sun peaking over planet

	//SaveScene();
	//ReadScene(L"wow");

	CreateLights();
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	{
		// Shutdown
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}


// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	m_EntityManager = EntityManager::GetInstance();

	AssetManager::InitializeSingleton(device, context);
	m_AssetManager = AssetManager::GetInstance();
	m_PhysicsManager = PhysXManager::GetInstance();

	{
		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		//io.Fonts->AddFontFromFileTTF("Arial.ttf", 13.0f);


		const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
		//load font with japanese support
		font = (*io.Fonts).AddFontFromFileTTF("E:/College Semester 7/IGME550/GameEngine/Assets/mplus-1m-regular.ttf", 13.0f , NULL, ranges);
		//io.Fonts->GetTexDataAsRGBA32();

		io.Fonts->Build();
		//io.Fonts->AddFontFromFileTTF("Arial.ttf", 13);
		
		
		// Pick a style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Initialize helper Platform and Renderer backends (here we are using imgui_impl_win32.cpp and imgui_impl_dx11.cpp)
		ImGui_ImplWin32_Init(this->hWnd);
		ImGui_ImplDX11_Init(device.Get(), context.Get());
	}



	//load create the shapes and skybox
	CreateBasicGeometry();

	//create the shadow resources
	//unique sampler and rasterizer state
	CreateShadowResources();

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Game::CreateBasicGeometry()
{
	std::vector<std::shared_ptr<Mesh>> meshes = m_AssetManager->GetMeshes();
	std::vector<std::shared_ptr<Material>> materials = m_AssetManager->GetMaterials();
  
	std::shared_ptr<SimpleVertexShader> vertexShader = m_AssetManager->GetVertexShader("vertexShader");
	std::shared_ptr<SimplePixelShader> debugPixelShader = m_AssetManager->GetPixelShader("debugPixelShader");
	//toon meshes
	toonMeshes.push_back(std::make_shared<Mesh>(m_AssetManager->GetFullPathTo("../../Assets/Models/Tree.obj").c_str(), device, context));

	m_AssetManager->AddSRVToMap(ToonAlbedo,		L"../../Assets/Toon/Lisa/Texture/服.png", true);
	m_AssetManager->AddSRVToMap(ToonRoughness,	L"../../Assets/Textures/noMetal.png", true);
	m_AssetManager->AddSRVToMap(ToonAO,			L"../../Assets/Textures/allMetal.png", true);
	m_AssetManager->AddSRVToMap(ToonMetalness,	L"../../Assets/Textures/noMetal.png", true);
	m_AssetManager->AddSRVToMap(SampleTexture,	L"../../Assets/Textures/Ramp_Texture.png", true);
	
	sabaLisa = m_AssetManager->GetSabaMesh(0);
	//sabaEntity = std::make_shared<GameEntity>(sabaLisa, m_AssetManager->GetToonMaterial(0), camera, device);
	//std::shared_ptr<Mesh> catapult = std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/catapult.obj").c_str(), device, context);
		
	//load animation file
	animFile = std::make_shared<saba::VMDFile>();
	saba::ReadVMDFile(animFile.get(), m_AssetManager->GetFullPathTo("../../Assets/Anim/Male_run_in_place_lisa.vmd").c_str());
	anim = std::make_shared<saba::VMDAnimation>();
	m_AssetManager->GetSabaMesh(0)->GetModel()->InitializeAnimation();
	anim->Create(m_AssetManager->GetSabaMesh(0)->GetModel());
	anim->Add(*animFile.get());

	//create some entities
	//cube direectly in front of camera
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[0], materials[0], camera, true));

	//Ground plane
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[4], materials[0], camera, m_PhysicsManager->CreateStatic(PxTransform(PxVec3(0, -10, 0)), PxBoxGeometry(20, .05, 20))));
	
	m_EntityManager->GetEntity(1)->GetTransform()->SetScale(20.0f, 1.0f, 20.0f);

	m_EntityManager->AddEntity(std::make_shared<GameEntity>(m_AssetManager->GetSabaMesh(0), camera, device, context,
		m_AssetManager->GetVertexShader("mmdVertexShader"), m_AssetManager->GetPixelShader("mmdPixelShader"),
		m_AssetManager->GetVertexShader("mmdEdgeVertexShader"), m_AssetManager->GetPixelShader("mmdEdgePixelShader")));

	m_EntityManager->GetEntity(2)->GetTransform()->MoveAbsolute(XMFLOAT3(0, 0, 20));
	m_EntityManager->GetEntity(2)->GetTransform()->Rotate(XMFLOAT3(0, XM_PI, 0));//face toward starting pos

	//allocate memory for morph weights
	morphWeights = std::make_shared<std::vector<float>>();

	std::shared_ptr<saba::PMXModel> model = m_AssetManager->GetSabaMesh(0)->GetModel();
	saba::MMDMorphManager* morphMan = model->GetMorphManager();
	size_t morphCount = morphMan->GetMorphCount();
	morphWeights->reserve(morphCount);
	for (size_t morphIdx = 0; morphIdx < morphCount; morphIdx++)
	{
		saba::MMDMorph* morph = morphMan->GetMorph(morphIdx);
		float weight = morph->GetWeight();
		morphWeights->push_back(weight);
	}


	//create sky obj
	sky = std::make_shared<Sky>(meshes[0], m_AssetManager->GetSampler("basicSampler"), m_AssetManager->GetSRV(SkyBox, 0), device, context, m_AssetManager->GetVertexShader("skyVertexShader"), m_AssetManager->GetPixelShader("skyPixelShader"));
}

void Game::CreateExtraRenderTargets()
{
	ID3D11Texture2D* backBufferTexture = 0;
	swapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		(void**)&backBufferTexture);

	// Now that we have the texture, create a render target view
	// for the back buffer so we can render into it.  Then release
	// our local reference to the texture, since we have the view.
	if (backBufferTexture != 0)
	{
		device->CreateRenderTargetView(
			backBufferTexture,
			0,
			backBufferRTV.GetAddressOf());
		backBufferTexture->Release();
	}

	// Set up the description of the texture to use for the depth buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;

	// Create the depth buffer and its view, then 
	// release our reference to the texture
	ID3D11Texture2D* depthBufferTexture = 0;
	device->CreateTexture2D(&depthStencilDesc, 0, &depthBufferTexture);
	if (depthBufferTexture != 0)
	{
		device->CreateDepthStencilView(
			depthBufferTexture,
			0,
			depthStencilView.GetAddressOf());
		depthBufferTexture->Release();
	}
}

void Game::CreateLights() {
	XMFLOAT3 white = { 1,1,1 };
	//create direction light
	Light temp = {};
	temp.Type = LIGHT_TYPE_DIRECTIONAL;
	temp.Position = XMFLOAT3(-20, 0, 0);//give direction lights position for shadows and light rays
	temp.Direction = XMFLOAT3(1, 0, 0); // point right 
	temp.Color = white;//XMFLOAT3(0, 0, 1);//bright blue 
	temp.Intensity = 1.0f; //each for testing right now
	temp.CastsShadows = false;

	lights.push_back(temp);

	/*temp.Direction = XMFLOAT3(0, -1, 0); // point directly 'down'
	temp.Color = XMFLOAT3(0, 0, 1);//bright blue
	temp.CastsShadows = false;

	lights.push_back(temp);*/

	//point light to cast shadow
	temp.Type = LIGHT_TYPE_POINT;
	temp.Color = XMFLOAT3(1, 1, 1); //white
	temp.Position = XMFLOAT3(0.0f, 2.0f, 0);
	temp.Direction = XMFLOAT3(0, 0, 0);
	temp.Range = 30;
	temp.Intensity = 0.25f;
	temp.NearZ = 0.5f;
	temp.FarZ = 100.0f;
	temp.CastsShadows = false;

	lights.push_back(temp);

	//test spot light
	temp.Type = LIGHT_TYPE_SPOT;
	temp.Color = XMFLOAT3(1, 0, 0);//red
	temp.Position = XMFLOAT3(0, 0, -2.5f);
	temp.Range = 10.0f;
	temp.Direction = XMFLOAT3(0, 0, 1); //directly toward starting camera pos
	temp.SpotFalloff = 20.0f;
	temp.Intensity = 1;
	temp.CastsShadows = true;

	lights.push_back(temp);

	for (int i = 0; i < lights.size(); i++)
	{
		lightPoses.push_back(lights[i].Position);
	}

}

void Game::CreateShadowResources()
{
	//set size of shadow tex and space in world
	shadowResolution = 2048;
	shadowProjSize = 30.0f;

	D3D11_TEXTURE2D_DESC shadowDesc = {};
	//set width and height of texture
	shadowDesc.Width = shadowResolution;
	shadowDesc.Height = shadowResolution;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;//only one color channel, use all the bits for it
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTex;
	device->CreateTexture2D(&shadowDesc, 0, shadowTex.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowSpotTex;
	device->CreateTexture2D(&shadowDesc, 0, shadowSpotTex.GetAddressOf());

	//create depth/stencil
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowStencilDesc = {};
	shadowStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowStencilDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTex.Get(), &shadowStencilDesc, shadowStencil.GetAddressOf());
	device->CreateDepthStencilView(shadowSpotTex.Get(), &shadowStencilDesc, shadowSpotStencil.GetAddressOf());

	//create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC shadowSRVDesc = {};
	shadowSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shadowSRVDesc.Texture2D.MipLevels = 1;
	shadowSRVDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTex.Get(), &shadowSRVDesc, shadowSRV.GetAddressOf());
	device->CreateShaderResourceView(shadowSpotTex.Get(), &shadowSRVDesc, shadowSpotSRV.GetAddressOf());

	//create special comparison smapler state
	D3D11_SAMPLER_DESC shadowSamplerDesc = {};
	shadowSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; //this is the comparision
	shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS; //how comparison functions
	shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;//use border color outside of tex area
	shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;//use border color outside of tex area
	shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;//use border color outside of tex area
	shadowSamplerDesc.BorderColor[0] = 1.0f;
	shadowSamplerDesc.BorderColor[1] = 1.0f;
	shadowSamplerDesc.BorderColor[2] = 1.0f;
	shadowSamplerDesc.BorderColor[3] = 1.0f;
	device->CreateSamplerState(&shadowSamplerDesc, shadowSampler.GetAddressOf());

	//create rasterizer state
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000;//'push' shadow back slightly to avoid flickering issue
	shadowRastDesc.DepthBiasClamp = 0.0;
	shadowRastDesc.SlopeScaledDepthBias = 3.0f; //3 works pretty well
	device->CreateRasterizerState(&shadowRastDesc, shadowRasterizer.GetAddressOf());


	D3D11_TEXTURE2D_DESC cubeDesc = {};
	cubeDesc.Width = shadowResolution; // Match the size
	cubeDesc.Height = shadowResolution; // Match the size
	cubeDesc.ArraySize = 6; // Cube map!
	cubeDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;; // We'll be using as a texture in a shader
	cubeDesc.CPUAccessFlags = 0; // No read back
	cubeDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Match the loaded texture's color format
	cubeDesc.MipLevels = 1; // Only need 1
	cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // A CUBE MAP, not 6 separate textures
	cubeDesc.SampleDesc.Count = 1;
	cubeDesc.SampleDesc.Quality = 0;
	cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage

	// Create the actual texture resource
	std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> cubeMapTextures;
	cubeMapTextures.resize(MAX_POINT_SHADOWS_NUM);

	shadowBoxStencils.resize(MAX_POINT_SHADOWS_NUM);
	shadowBoxSRVs.resize(MAX_POINT_SHADOWS_NUM);
	//run twice to set up two point maps
	for (int i = 0; i < MAX_POINT_SHADOWS_NUM; i++) {
		device->CreateTexture2D(&cubeDesc, 0, cubeMapTextures[i].GetAddressOf());

		for (int j = 0; j < 6; j++) {
			//create depth/stencil
			D3D11_DEPTH_STENCIL_VIEW_DESC shadowBoxStencilDesc = {};
			shadowBoxStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
			shadowBoxStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			shadowBoxStencilDesc.Texture2DArray.MipSlice = 0;
			shadowBoxStencilDesc.Texture2DArray.ArraySize = 1;
			shadowBoxStencilDesc.Texture2DArray.FirstArraySlice = j;

			shadowBoxStencils[i].push_back(nullptr);
			device->CreateDepthStencilView(cubeMapTextures[i].Get(), &shadowBoxStencilDesc, shadowBoxStencils[i][j].GetAddressOf());
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
		srvDesc.TextureCube.MipLevels = 1; // Only need access to 1 mip
		srvDesc.TextureCube.MostDetailedMip = 0; // Index of the first mip we want to see
		device->CreateShaderResourceView(cubeMapTextures[i].Get(), &srvDesc, shadowBoxSRVs[i].GetAddressOf());

	}
}

void Game::RenderDirectionalShadowMap(XMFLOAT3 dir, XMFLOAT3 targetPos)
{
	std::shared_ptr<SimpleVertexShader> shadowVertexShader = m_AssetManager->GetVertexShader("shadowVertexShader");

	//move 20 units back along view direction
	//create "camera" mats
	XMMATRIX shView = XMMatrixLookAtLH(
		XMVectorSet(dir.x * -20 + targetPos.x, dir.y * -20 + targetPos.y, dir.z * -20 + targetPos.z, 0),
		XMVectorSet(targetPos.x, targetPos.y, targetPos.z, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&shadowViewMat, shView);

	//use perspective for point light shadows
	XMMATRIX shProj = XMMatrixOrthographicLH(shadowProjSize, shadowProjSize, 0.1f, 100.0f);
	XMStoreFloat4x4(&shadowProjMat, shProj);

	//unbind shadow resource
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	//shadow map is slot 5 in ps
	context->PSSetShaderResources(5, 1, pSRV);

	//setup pipline for shadow map
	context->OMSetRenderTargets(0, 0, shadowStencil.Get());
	context->ClearDepthStencilView(shadowStencil.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->RSSetState(shadowRasterizer.Get());

	//Create viewport that matches shadow map resolution
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)shadowResolution;
	vp.Height = (float)shadowResolution;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowProjMat);
	//turn off ps
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	// '&' is important because it prevents making copies
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}



	//reset all render states
	//context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	context->OMSetRenderTargets(1, middleBufferRTV.GetAddressOf(), middleDepthStencilView.Get());
	vp.Width = (float)this->width;
	vp.Height = (float)this->height;
	context->RSSetViewports(1, &vp);
	context->RSSetState(0); //reset
}

void Game::RenderPointShadowMap(DirectX::XMFLOAT3 pos, int index, float range, float nearZ, float farZ)
{
	std::shared_ptr<SimpleVertexShader> shadowVertexShader = m_AssetManager->GetVertexShader("shadowVertexShader");

	//unbind shadow resource
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	context->PSSetShaderResources(7 + index, 1, pSRV); //shadow map starts at 7th

	//setup pipline for shadow map
	context->OMSetRenderTargets(0, 0, shadowBoxStencils[index][0].Get());
	context->ClearDepthStencilView(shadowBoxStencils[index][0].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->RSSetState(shadowRasterizer.Get());

	//find which entities are in range to be rendered to map
	renderableEntities.clear(); //clear previous culling
	//loop and draw all objects in range of shadow
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		DirectX::XMFLOAT3 ePos = m_EntityManager->GetEntity(i)->GetTransform()->GetPosition();
		//get square dist cause faster
		float squareDist = powf(pos.x - ePos.x, 2) + powf(pos.y - ePos.y, 2) + powf(pos.z - ePos.z, 2);
		if (squareDist < powf(farZ, 2)) {
			renderableEntities.push_back(m_EntityManager->GetEntity(i));
		}
	}

	//Create viewport that matches shadow map resolution
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)shadowResolution;
	vp.Height = (float)shadowResolution;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	DirectX::XMFLOAT4X4 shadowBoxViewMat;
	DirectX::XMFLOAT4X4 shadowBoxProjMat;

	//render the right side
	//create "camera" mats
	//use perspective for point light shadows
	XMMATRIX shProj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, nearZ, farZ);
	XMStoreFloat4x4(&shadowBoxProjMat, shProj);

#pragma region Right

	XMMATRIX shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),//start at origin to match point like
		XMVectorSet(1, 0, 0, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&shadowBoxViewMat, shView);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	// '&' is important because it prevents making copies
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}

#pragma endregion

	//left side
#pragma region Left

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[index][1].Get());
	context->ClearDepthStencilView(shadowBoxStencils[index][1].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),//start at origin to match point like
		XMVectorSet(-1, 0, 0, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&shadowBoxViewMat, shView);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);
    
		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}

#pragma endregion

#pragma region Up

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[index][2].Get());
	context->ClearDepthStencilView(shadowBoxStencils[index][2].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),//start at origin to match point like
		XMVectorSet(0, 1, 0, 0),
		XMVectorSet(0, 0, -1, 0)); //need to change up direction b/c can't be same dir as look
	XMStoreFloat4x4(&shadowBoxViewMat, shView);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}

#pragma endregion

#pragma region Down

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[index][3].Get());
	context->ClearDepthStencilView(shadowBoxStencils[index][3].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),//start at origin to match point like
		XMVectorSet(0, -1, 0, 0),
		XMVectorSet(0, 0, 1, 0));
	XMStoreFloat4x4(&shadowBoxViewMat, shView);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	// '&' is important because it prevents making copies

	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}

#pragma endregion

#pragma region Front

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[index][4].Get());
	context->ClearDepthStencilView(shadowBoxStencils[index][4].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),//start at origin to match point like
		XMVectorSet(0, 0, 1, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&shadowBoxViewMat, shView);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}

#pragma endregion

#pragma region Back

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[index][5].Get());
	context->ClearDepthStencilView(shadowBoxStencils[index][5].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),//start at origin to match point like
		XMVectorSet(0, 0, -1, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&shadowBoxViewMat, shView);

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}

#pragma endregion

	//clear render target
	ID3D11RenderTargetView* nullView = NULL;
	context->OMSetRenderTargets(1, &nullView, NULL);

	//reset all render states
	//context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	context->OMSetRenderTargets(1, middleBufferRTV.GetAddressOf(), middleDepthStencilView.Get());
	vp.Width = (float)this->width;
	vp.Height = (float)this->height;
	context->RSSetViewports(1, &vp);
	context->RSSetState(0); //reset
}

void Game::RenderSpotShadowMap(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, float range, float spotFallOff, float nearZ, float farZ)
{

	//unbind shadow resource
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	context->PSSetShaderResources(6, 1, pSRV); //spot shadow map is 6th

	//create "camera" mats
	XMMATRIX shView = XMMatrixLookToLH(
		XMVectorSet(pos.x, pos.y, pos.z, 0),
		XMVectorSet(dir.x, dir.y, dir.z, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&spotShadowViewMat, shView);

	//light.range is adjacent side, half spot fall off is the opposit side
	//so tan(theta) = opposite/adjacent, then aadjacent * tan(theta) = opposite, which would be 
	//max projection size
	//float projSize = range * tanf((spotFallOff / 2.0f) * toRadians);

	float yAngle = (spotFallOff / 2.0f) / range;
	//use perspective for point light shadows
	XMMATRIX shProj = XMMatrixPerspectiveFovLH(90.0f * toRadians, 1.0f, nearZ, farZ);// XMMatrixPerspectiveLH(spotFallOff, spotFallOff, nearZ, farZ);
	XMStoreFloat4x4(&spotShadowProjMat, shProj);

	//setup pipline for shadow map
	context->OMSetRenderTargets(0, 0, shadowSpotStencil.Get());
	context->ClearDepthStencilView(shadowSpotStencil.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->RSSetState(shadowRasterizer.Get());

	//Create viewport that matches shadow map resolution
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)shadowResolution;
	vp.Height = (float)shadowResolution;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	std::shared_ptr<SimpleVertexShader> shadowVertexShader = m_AssetManager->GetVertexShader("shadowVertexShader");

	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", spotShadowViewMat);
	shadowVertexShader->SetMatrix4x4("proj", spotShadowProjMat);
	//turn off ps
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	// '&' is important because it prevents making copies
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}



	//reset all render states
	//context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	context->OMSetRenderTargets(1, middleBufferRTV.GetAddressOf(), middleDepthStencilView.Get());
	vp.Width = (float)this->width;
	vp.Height = (float)this->height;
	context->RSSetViewports(1, &vp);
	context->RSSetState(0); //reset
}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	if (camera != nullptr) {
		camera->UpdateProjectionMatrix((float)this->width / this->height);
	}
}

void Game::CreateGui(float deltaTime) {
	Input& input = Input::GetInstance();

	{
		// Reset input manager's gui state
		// so we don't taint our own input
		input.SetGuiKeyboardFocus(false);
		input.SetGuiMouseFocus(false);

		// Set io info
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = deltaTime;
		io.DisplaySize.x = (float)this->width;
		io.DisplaySize.y = (float)this->height;
		io.KeyCtrl = input.KeyDown(VK_CONTROL);
		io.KeyShift = input.KeyDown(VK_SHIFT);
		io.KeyAlt = input.KeyDown(VK_MENU);
		io.MousePos.x = (float)input.GetMouseX();
		io.MousePos.y = (float)input.GetMouseY();
		io.MouseDown[0] = input.MouseLeftDown();
		io.MouseDown[1] = input.MouseRightDown();
		io.MouseDown[2] = input.MouseMiddleDown();
		io.MouseWheel = input.GetMouseWheel();
		input.GetKeyArray(io.KeysDown, 256);

		// Reset the frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		input.SetGuiKeyboardFocus(io.WantCaptureKeyboard);
		input.SetGuiMouseFocus(io.WantCaptureMouse);

		//Keeps track of the fps in the ImGui UI
		static float timer = 0.0f;
		timer += deltaTime;
		static int frameCount = 0;
		static int lastFrameCount = frameCount;

		frameCount++;
		if (timer > 1.0f) {
			timer = 0.0f;
			lastFrameCount = frameCount;
			frameCount = 0;
		}

		ImGui::Text("FPS: %i", lastFrameCount);

		ImGui::PushID(1);

		bool entitiesOpen = ImGui::TreeNode("Entities", "%s", "Entities");
		if (entitiesOpen) {
			std::unordered_map<Transform*, std::shared_ptr<GameEntity>> childEntityTransformMap;

			//Lambda function cause I thought it'd be cool but as it turns out it was more of a hassle than it was worth lol
			static auto addEntity = [&](auto&& addEntity, Transform* entityTransform, int entityNum, std::shared_ptr<GameEntity> currEntity) {

				ImGui::PushID(entityTransform);
				bool nodeOpen = ImGui::TreeNode("Entity", "%s %i", "Entity", entityNum);
				if (!nodeOpen) {
					ImGui::PopID();
					return;
				}

				bool drawSphere = currEntity->ShouldDrawSphere();
				ImGui::Text("Draw Bounding Sphere: ");
				ImGui::SameLine();
				ImGui::Checkbox("   ", &drawSphere);
				currEntity->SetDrawSphere(drawSphere);

				bool drawCube = currEntity->ShouldDrawCube();
				ImGui::Text("Draw Bounding Cube: ");
				ImGui::SameLine();
				ImGui::Checkbox("    ", &drawCube);
				currEntity->SetDrawCube(drawCube);

				//Allows for control over position of entities
				DirectX::XMFLOAT3 position = entityTransform->GetPosition();
				ImGui::Text("Position: ");
				ImGui::SameLine();
				ImGui::DragFloat3("", &position.x, .05f, -D3D11_FLOAT32_MAX, D3D11_FLOAT32_MAX);
				entityTransform->SetPosition(position);

				//Allows for control over rotation of entities
				DirectX::XMFLOAT3 rotation = entityTransform->GetEulerAngles();
				ImGui::Text("Rotation: ");
				ImGui::SameLine();
				ImGui::DragFloat3(" ", &rotation.x, .05f, -DirectX::XM_PI, DirectX::XM_PI);
				entityTransform->SetRotation(rotation);

				//Allows for control over scale of entities
				DirectX::XMFLOAT3 scale = entityTransform->GetScale();
				ImGui::Text("Scale: ");
				ImGui::SameLine();
				ImGui::DragFloat3("  ", &scale.x, .25f, 0.0f, D3D11_FLOAT32_MAX);
				entityTransform->SetScale(scale);

				int numChildren = entityTransform->GetNumChildren();
				//First transform is debug sphere
				for (int i = 1; i < numChildren; i++)
				{
					Transform* tempTransform = entityTransform->GetChild(i);
					//Currently bounding sphere's will be limited to the top level entity in the tree
					if (childEntityTransformMap[tempTransform])
					{
						addEntity(addEntity, tempTransform, i, childEntityTransformMap[tempTransform]);
					}
				}

				ImGui::TreePop();
				ImGui::PopID();
			};

			int entityID = 1;
			for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
				std::shared_ptr<GameEntity> currEntity = m_EntityManager->GetEntity(i);

				if (currEntity->GetTransform()->GetParent()) {
					childEntityTransformMap[currEntity->GetTransform()] = currEntity;
				}
			}

			for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
				std::shared_ptr<GameEntity> currEntity = m_EntityManager->GetEntity(i);

				if (currEntity->GetTransform()->GetParent()) {
					continue;
				}

				addEntity(addEntity, currEntity->GetTransform(), entityID, currEntity);

				entityID++;
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		/*
		ImGui::PushID(2);
		bool shadowsOpen = ImGui::TreeNode("Shadows", "%s", "Shadows");
		if (shadowsOpen)
		{
			/*ImGui::Text("Resolution: ");
			ImGui::SameLine();
			int resolution = m_shadowManager->GetResolution();
			int prevResolution = resolution;
			ImGui::DragInt("resolution", &resolution, 8, 16, 16384);
			if (resolution != prevResolution)
			{
				m_shadowManager->SetResolution(resolution);
			}
			ImGui::Text("Dist From Center: ");
			ImGui::SameLine();
			ImGui::DragFloat(" ", &dirLightShadowDistFromOrigin, 0.05f, 1.0f, 50.0f);

			ImGui::Text("Projection Size: ");
			ImGui::SameLine();
			float projectionSize = shadow->GetProjectionSize();
			float prevProjectionSize = projectionSize;
			ImGui::DragFloat("   ", &projectionSize, 1.0f, 1.0f, 512.0f);
			if (projectionSize != prevProjectionSize)
			{
				m_shadowManager->SetProjectionSize(projectionSize);
			}



			// Determine new input capture
			//input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
			//input.SetGuiMouseCapture(io.WantCaptureMouse);
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
			ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
			ImGui::Image(m_shadowManager->GetShadowSRV().Get(), ImVec2(256, 256), uv_min, uv_max, tint_col, border_col);

			ImGui::TreePop();
		}
		ImGui::PopID();
		*/

		ImGui::PushID(2);
		bool lightsOpen = ImGui::TreeNode("Lights", "%s", "Lights");
		if (lightsOpen)
		{
			//ImGui::Text("Num Lights");
			//ImGui::SameLine();
			//ImGui::SliderInt("             ", &numLightsToRender, 0, static_cast<int>(m_lights.size()));

			for (int i = 0; i < lights.size(); i++) {
				Light& currLight = lights[i];

				//Since this isn't a pointer I want a new guid rather than mem address
				static GUID lightID;
				HRESULT guidCreationRes;
				if (lightID.Data1 == 0)
					guidCreationRes = CoCreateGuid(&lightID);

				ImGui::PushID(static_cast<int>(lightID.Data1) + i);

				bool nodeOpen = ImGui::TreeNode("Light", "%s %i", "Light", i + 1);
				if (nodeOpen) {
					const char* items[] = { "Directional", "Point", "Spot" };
					ImGui::Text("Light Type: ");
					ImGui::SameLine();
					ImGui::Combo("        ", &currLight.Type, items, IM_ARRAYSIZE(items));

					switch (currLight.Type)
					{
					case 0:
						ImGui::Text("Direction: ");
						ImGui::SameLine();
						ImGui::DragFloat3(" ", &currLight.Direction.x, .01f, -1.0f, 1.0f);

						//I would use casts shadows here but for some reason the bool is being turned to false
						//if (i == 0)
						//	m_shadowManager->SetLightPosition(DirectX::XMFLOAT3(currLight.Direction.x * dirLightShadowDistFromOrigin, currLight.Direction.y * dirLightShadowDistFromOrigin, currLight.Direction.z * dirLightShadowDistFromOrigin));

						break;

					case 1:
						ImGui::Text("Position: ");
						ImGui::SameLine();
						ImGui::DragFloat3(" ", &currLight.Position.x, .1f, -D3D11_FLOAT32_MAX, D3D11_FLOAT32_MAX);

						ImGui::Text("Range: ");
						ImGui::SameLine();
						ImGui::DragFloat("  ", &currLight.Range, 0.1f, 0.1f, D3D11_FLOAT32_MAX);
						break;

					case 2:
						ImGui::Text("Direction: ");
						ImGui::SameLine();
						ImGui::DragFloat3(" ", &currLight.Direction.x, .01f, -1.0f, 1.0f);

						ImGui::Text("Spot Falloff: ");
						ImGui::SameLine();
						ImGui::DragFloat("  ", &currLight.SpotFalloff);
						break;
					default:
						break;
					}

					ImGui::Text("Intensity: ");
					ImGui::SameLine();
					ImGui::DragFloat(" ", &currLight.Intensity, .01f, 0.01f, 2.0f);

					ImGui::Text("Color: ");
					ImGui::SameLine();
					ImGui::ColorEdit3(" ", &currLight.Color.x);

					ImGui::Text("Casts Shadows: ");
					ImGui::SameLine();
					ImGui::Checkbox("   ", &currLight.CastsShadows);

					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}
		ImGui::PopID();

		ImGui::PushID(3);
		bool raysOpen = ImGui::TreeNode("God Rays", "%s", "God Rays");
		if (raysOpen)
		{
			//ImGui::Text("Light Rays");

			ImGui::Text("Enable Light Rays: ");
			ImGui::SameLine();
			ImGui::Checkbox("   ", &enableLightRays);

			if (enableLightRays)
			{
				ImGui::Text("Density: ");
				ImGui::SameLine();
				ImGui::DragFloat(" ", &lightRaysDensity, .01f, 0.01f, 1.0f);

				ImGui::Text("Weight: ");
				ImGui::SameLine();
				ImGui::DragFloat("  ", &lightRaysWeight, .01f, 0.01f, 1.0f);

				ImGui::Text("Decay: ");
				ImGui::SameLine();
				ImGui::DragFloat("    ", &lightRaysDecay, .01f, 0.01f, 1.0f);

				ImGui::Text("Exposure: ");
				ImGui::SameLine();
				ImGui::DragFloat("     ", &lightRaysExposure, .01f, 0.01f, 1.0f);
			}
			
			ImGui::TreePop();
		}
		ImGui::PopID();

		ImGui::PushID(4);

		if (camera) {
			ImGui::Text("Move Speed: ");
			ImGui::SameLine();
			ImGui::DragFloat(" ", camera->GetMoveSpeed(), .01f, 0.01f, 10.0f);

			ImGui::Text("Mouse Move Speed: ");
			ImGui::SameLine();
			ImGui::DragFloat("  ", camera->GetMouseMoveSpeed(), .01f, 0.01f, 2.0f);
		}

		ImGui::PopID();

		ImGui::PushID(5);

		//ImGui::PushFont(font);

		ImGui::Text("Enable animation: ");
		ImGui::SameLine();
		ImGui::Checkbox("    ", &animOn);
		if (animOn) {
			ImGui::Text("Enable Run: ");
			ImGui::SameLine();
			ImGui::Checkbox("     ", &runAnim);
			ImGui::Text("Enable Morph: ");
			ImGui::SameLine();
			ImGui::Checkbox("      ", &morphAnim);
		}
		// Show the demo window
		//ImGui::ShowDemoWindow();	
		if (morphAnim && ImGui::TreeNode("Morph"))
		{
			auto model = m_AssetManager->GetSabaMesh(0)->GetModel();
			auto morphMan = model->GetMorphManager();
			size_t morphCount = morphMan->GetMorphCount();
			for (size_t morphIdx = 0; morphIdx < morphCount; morphIdx++)
			{
				auto morph = morphMan->GetMorph(morphIdx);
				//printf("%zi, %s", morphIdx, morph->GetName());
				//std::cout << "ID: " << morphIdx << ", Name: " << morph->GetName() << std::endl;
				ImGui::SliderFloat(morph->GetName().c_str(), &(*morphWeights)[morphIdx], 0.0f, 1.0f);
			}
			ImGui::TreePop();
		}
		//ImGui::PopFont();
		ImGui::PopID();
	}
}

void Game::CreateMaterialGUI(float deltaTime) {
	Input& input = Input::GetInstance();

	{
		// Set io info
		ImGuiIO& io = ImGui::GetIO();
		input.GetKeyArray(io.KeysDown, 256);

		ImGui::Begin("Create a new Material");

		// Determine new input capture
		input.SetGuiKeyboardFocus(io.WantCaptureKeyboard);
		input.SetGuiMouseFocus(io.WantCaptureMouse);

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
		static int selectedTextureIndex = 0;

		ImGui::PushID(5);
		ImGui::Text("Texture Index");
		ImGui::SameLine();
		ImGui::DragInt(" ", &selectedTextureIndex, 1, static_cast<int>(0), 3);//m_AssetManager->sr.size()));
		ImGui::Image(m_AssetManager->GetSRV(Albedo, selectedTextureIndex).Get(), ImVec2(256, 256), uv_min, uv_max, tint_col, border_col);
		
		ImGui::PopID();
		ImGui::End();
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{

	//m_AssetManager->GetSabaMesh(0)->GetModel()->UpdateMorphAnimation();
	if (animOn) {
		float time = (float)saba::GetTime();
		float elapsed = time - saveTime;
		if (elapsed > 1.0 / 30.0)
		{
			elapsed = 1.0 / 30.0;
		}
		saveTime = time;
		animTime += float(elapsed);
		std::shared_ptr<saba::PMXModel> tempModel = m_AssetManager->GetSabaMesh(0)->GetModel();
		tempModel->BeginAnimation();
		if (runAnim) {
			tempModel->UpdateAllAnimation(anim.get(), animTime * 30.0f, elapsed);
		}
		if (morphAnim) {
			auto morphManager = m_AssetManager->GetSabaMesh(0)->GetModel()->GetMorphManager();
			for (int i = 0; i < morphWeights->size(); i++) {
				morphManager->GetMorph(i)->SetWeight((*morphWeights)[i]);
			}
			//morph->SetWeight(1.0);
			tempModel->UpdateMorphAnimation();
		}
		tempModel->EndAnimation();
		tempModel->Update();
		size_t vtxCount = tempModel->GetVertexCount();
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE mapRes;
		hr = context->Map(sabaLisa->GetVertexBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRes);
		if (FAILED(hr))
		{
			return;
		}
		Vertex* vertices = (Vertex*)mapRes.pData;
		const glm::vec3* positions = tempModel->GetUpdatePositions();
		const glm::vec3* normals = tempModel->GetUpdateNormals();
		const glm::vec2* uvs = tempModel->GetUpdateUVs();
		for (size_t i = 0; i < vtxCount; i++)
		{
			vertices[i].Position = XMFLOAT3(positions[i][0], positions[i][1], positions[i][2]);
			vertices[i].Normal = XMFLOAT3(normals[i][0], normals[i][1], normals[i][2]);
			vertices[i].UVCoord = XMFLOAT2(uvs[i][0], uvs[i][1]);
			vertices[i].Tangent = {};
		}
		context->Unmap(sabaLisa->GetVertexBuffer().Get(), 0);
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE)) {
		Quit();
	}

	//make objects move, scale, or rotate
	//m_EntityManager->GetEntity(0)->GetTransform()->MoveRelative(XMFLOAT3(-sin(totalTime * 2.0f) * 0.25f, 0, 0));
	m_EntityManager->UpdateEntities(deltaTime);
	m_PhysicsManager->UpdatePhysics(deltaTime);

	CreateGui(deltaTime);
	//CreateMaterialGUI(deltaTime);

	camera->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };
	
	//clear render target
	ID3D11RenderTargetView* pNullRTV = NULL;
	context->OMSetRenderTargets(1, &pNullRTV, NULL);

	//unbind slot 0 which is where we send the middle process tex
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	context->PSSetShaderResources(0, 1, pSRV);

	//draw to secondary render target so we can do post process effects
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	if (enableLightRays) {
		//draw to secondary render target so we can do post process effects
		context->OMSetRenderTargets(1, middleBufferRTV.GetAddressOf(), middleDepthStencilView.Get());

		context->ClearRenderTargetView(middleBufferRTV.Get(), color);
		context->ClearDepthStencilView(
			middleDepthStencilView.Get(),
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
			1.0f,
			0);
	}

	//stuff above needed every frame.


	//make sure we only render two point maps at max
	int numPointMaps = 0;
	//do shadow rendering stuff
	for (int i = 0; i < lights.size(); i++) {
		//lights[i].ShadowNumber = -1; //set to -1 so ps knows that it's doesn't use point shadow map
		if (lights[i].CastsShadows) {
			if (lights[i].Type == LIGHT_TYPE_DIRECTIONAL) {
				RenderDirectionalShadowMap(lights[i].Direction, m_EntityManager->GetEntity(2)->GetTransform()->GetPosition());
			}
			else if (lights[i].Type == LIGHT_TYPE_POINT && numPointMaps < MAX_POINT_SHADOWS_NUM) {
				RenderPointShadowMap(lights[i].Position, numPointMaps, lights[i].Range, lights[i].NearZ, lights[i].FarZ);
				//lights[i].ShadowNumber = numPointMaps;
				numPointMaps++;
			}
			else if (lights[i].Type == LIGHT_TYPE_SPOT) {
				RenderSpotShadowMap(lights[i].Position, lights[i].Direction, lights[i].Range, lights[i].SpotFalloff, lights[i].NearZ, lights[i].FarZ);
			}
		}
	}

	if (enableLightRays) {
		//draw game entities
		//draw to secondary render target so we can do post process effects
		context->OMSetRenderTargets(1, middleBufferRTV.GetAddressOf(), middleDepthStencilView.Get());
	}
	else
	{
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	}
	
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		//unbind slot 0 which is where we send the middle process tex
		//unbind all slots
		for (int j = 0; j < 9; j++) {
			context->PSSetShaderResources(j, 1, pSRV);
		}

		if (entity->GetMesh()->IsPmx()) {
			std::shared_ptr<AssetManager> assetManager = AssetManager::GetInstance();

			std::shared_ptr<SimpleVertexShader> vs = assetManager->GetSabaMaterials()[0]->GetVertexShader();
			//send shadow info to vertex shader
			vs->SetMatrix4x4("lightView", shadowViewMat);
			//vs->SetData("lightView", &sh)
			vs->SetMatrix4x4("lightProj", shadowProjMat);
			vs->SetMatrix4x4("spotLightView", spotShadowViewMat);
			vs->SetMatrix4x4("spotLightProj", spotShadowProjMat);

			std::shared_ptr<SimplePixelShader> ps = assetManager->GetSabaMaterials()[0]->GetPixelShader();
			ps->SetShaderResourceView("ShadowMap", shadowSRV);
			ps->SetShaderResourceView("ShadowBox1", shadowBoxSRVs[0]);
			ps->SetShaderResourceView("ShadowBox2", shadowBoxSRVs[1]);
			ps->SetShaderResourceView("ShadowSpotMap", shadowSpotSRV);
			ps->SetSamplerState("ShadowSampler", shadowSampler);

			//send light data to shaders
			entity->DrawPMX(entity->GetTransform()->GetWorldMatrix(), camera->GetViewMatrix(), camera->GetProjectionMatrix(), 
				/*lights[0].Color, lights[0].Direction,*/(int)lights.size(), &lights[0],
				backBufferRTV, depthStencilView, width, height);
		}
		else {

			std::shared_ptr<SimpleVertexShader> vs = entity->GetMaterial()->GetVertexShader();
			//send shadow info to vertex shader
			vs->SetMatrix4x4("lightView", shadowViewMat);
			//vs->SetData("lightView", &sh)
			vs->SetMatrix4x4("lightProj", shadowProjMat);
			vs->SetMatrix4x4("spotLightView", spotShadowViewMat);
			vs->SetMatrix4x4("spotLightProj", spotShadowProjMat);
			/*for (int j = 0; j < lights.size(); j++) {
				if (lights[j].Type == LIGHT_TYPE_POINT) {
					lightPoses[j] = lights[j].Position;
				}
			}*/

			//vs->SetData("lightPoses", &lightPoses[0], sizeof(XMFLOAT3) * (int)lightPoses.size());

			std::shared_ptr<SimplePixelShader> ps = entity->GetMaterial()->GetPixelShader();
			//send light data to shaders
			ps->SetInt("numLights", static_cast<int>(lights.size()));
			ps->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
			ps->SetShaderResourceView("ShadowMap", shadowSRV);
			ps->SetShaderResourceView("ShadowBox1", shadowBoxSRVs[0]);
			ps->SetShaderResourceView("ShadowBox2", shadowBoxSRVs[1]);
			//ps->SetShaderResourceView("ShadowBox", &shadowBoxSRVs[0]);// , sizeof(shadowBoxSRVs)* (int)shadowBoxSRVs.size());
			//context->PSSetShaderResources(7, 1, &shadowBoxSRVs[0]);
			//context->PSSetShaderResources(8, 1, &shadowBoxSRVs[1]);
			ps->SetShaderResourceView("ShadowSpotMap", shadowSpotSRV);

			ps->SetFloat3("ambientTerm", ambientTerm);
			ps->SetSamplerState("ShadowSampler", shadowSampler);
			entity->GetMaterial()->PrepareMaterial();

			entity->Draw();
		}
	}

	//sabaEntity->Draw();

	//draw sky, after everthying else to reduce overdraw
	sky->Draw(camera);

	if (enableLightRays)
	{
		//unbind slot 0 which is where we send the middle process tex
		context->PSSetShaderResources(0, 1, pSRV);

		//switch render target back to main to draw post process effects
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());

		std::shared_ptr<SimpleVertexShader> ppLightRaysVertexShader = m_AssetManager->GetVertexShader("ppLightRaysVertexShader");
		std::shared_ptr<SimplePixelShader> ppLightRaysPixelShader = m_AssetManager->GetPixelShader("ppLightRaysPixelShader");
	
		//matrix world;
		//matrix worldInvTranspose
		//matrix proj;
		//matrix view;
		//float4 lightPos;
		ppLightRaysVertexShader->SetShader();
		ppLightRaysPixelShader->SetShader();

		Transform lightWorldMat = Transform();
		lightWorldMat.MoveAbsolute(lights[0].Direction.x * -20, lights[0].Direction.y * -20, lights[0].Direction.z * -20);
		//add rotation to mat
		//lightWorldMat.SetRotation(lights[0].Direction); //fix this, find rotation to point toward this direction

		//load camera transform because XMLoadFloat has to take a l-value
		XMFLOAT3 cameraPos = camera->GetTransform()->GetPosition();
		XMFLOAT3 cameraForward = camera->GetTransform()->GetForward();

		//get dot product between camera forward vector and direction to sun
		//lights[0] is sun
		XMVECTOR dirToLight = XMLoadFloat3(&lights[0].Position) - XMLoadFloat3(&cameraPos);
		float dot = 0;
		XMStoreFloat(&dot, XMVector3Dot(XMVector3Normalize(dirToLight), XMVector3Normalize(XMLoadFloat3(&cameraForward))));


		//printf("dot product: %f\n", dot);
		//if the dot product is negative scale the density of the light rays by 1 - abs(dot)
		//if it's positive don't scale density. ie dot = 1
		if (dot < 0) {
			dot = 0;
		}

		
		float density = (float)pow(lightRaysDensity, 1/(dot+0.001));
		printf("scale amount : % f\n", density);

		ppLightRaysVertexShader->SetMatrix4x4("world", lightWorldMat.GetWorldMatrix());
		//ppLightRaysVertexShader->SetMatrix4x4("worldInverseTranspose", );
		ppLightRaysVertexShader->SetMatrix4x4("view", camera->GetViewMatrix());
		ppLightRaysVertexShader->SetMatrix4x4("proj", camera->GetProjectionMatrix());
		ppLightRaysVertexShader->SetFloat3("lightPos", lights[0].Position);


		//float density;
		//float weight;
		//float decay;
		//float exposure;
		ppLightRaysPixelShader->SetFloat3("lightColor", lights[0].Color);
		ppLightRaysPixelShader->SetFloat("density", density);
		ppLightRaysPixelShader->SetFloat("weight", lightRaysWeight);
		ppLightRaysPixelShader->SetFloat("decay", lightRaysDecay);
		ppLightRaysPixelShader->SetFloat("exposure", lightRaysExposure);

		ppLightRaysPixelShader->SetShaderResourceView("ScreenTexture", middleBufferSRV.Get());
		ppLightRaysPixelShader->SetShaderResourceView("ShadowMap", shadowSRV);
		ppLightRaysPixelShader->SetSamplerState("BasicSampler", m_AssetManager->GetSampler("ppLightRaysSampler"));
		ppLightRaysPixelShader->SetSamplerState("ShadowSampler", shadowSampler);

		ppLightRaysVertexShader->CopyAllBufferData();
		ppLightRaysPixelShader->CopyAllBufferData();

		context->Draw(3, 0);
	}

	{
		// Render dear imgui into screen
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	//stuff below unrelated to things being drawn

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(vsync ? 1 : 0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	//context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	//context->OMSetRenderTargets(1, middleBufferRTV.GetAddressOf(), middleDepthStencilView.Get());
}

///Helper function to reduce amount of typing in shadow map functions.
void PassShadowObjs() {
	/*
	//turn on our special shadow shader
	shadowVertexShader->SetShader();
	//the view and proj will be the same for all objects
	shadowVertexShader->SetMatrix4x4("view", shadowBoxViewMat);
	shadowVertexShader->SetMatrix4x4("proj", shadowBoxProjMat);
	shadowVertexShader->SetFloat3("lightPos", pos);
	//custom ps that outputs linear depth
	//shadowPixelShader->SetShader();
	context->PSSetShader(0, 0, 0);

	//loop and draw all objects in range of shadow
	// '&' is important because it prevents making copies
	for (auto& entity : renderableEntities) {
		//set this game entity's world mat, send to gpu
		shadowVertexShader->SetMatrix4x4("world", entity->GetTransform()->GetWorldMatrix());
		//copy data over
		shadowVertexShader->CopyAllBufferData();

		//draw. Don't need material
		entity->GetMesh()->Draw();
	}
	*/
}

void Game::Save() {
	WriteToFile("TestFile.txt", m_AssetManager->GetMaterial(0)->SerializeToString());
}

void Game::WriteToFile(std::string name, std::string writeString) {
	std::ofstream wStream(name, std::ios::out);
	if (!wStream) {
		std::cout << "Cannot open " << name << "" << std::endl;
	}

	wStream.write(writeString.c_str(), sizeof(char) * writeString.length());

	wStream.close();
	if (!wStream.good()) {
		std::cout << "Error occurred at write time" << std::endl;
	}
}
void Game::SaveScene() {
	std::ofstream wStream("Test.ncscn", std::ios::out | std::ios::binary);
	if (!wStream) {
		std::cout << "Cannot open file!" << std::endl;
	}

	wStream.write((char*) "Temp\0", sizeof(char[5]));

	wStream.close();
	if (!wStream.good()) {
		std::cout << "Error occurred at writing time!" << std::endl;
	}
}

void Game::ReadScene(std::wstring path) {
	std::ifstream rStream("Test.ncscn", std::ios::out | std::ios::binary);
	if (!rStream) {
		std::cout << "Cannot open file!" << std::endl;
	}
	char* temp = new char[5];
	rStream.read((char*)&temp[0], sizeof(char[5]));
	rStream.close();
	if (!rStream.good()) {
		std::cout << "Error occurred at reading time!" << std::endl;
	}

	std::cout << temp << " success!" << std::endl;
	delete[] temp;
}