#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "BufferStructs.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "WICTextureLoader.h" //loading textures, in DirectX namespace

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

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
	camera = std::make_shared<Camera>(XMFLOAT3(0.0f, 5.0f, -5.0f), (float)this->width / this->height, XM_PIDIV4, 0.1f, 1000.0f);
	camera->GetTransform()->Rotate(XMFLOAT3(XM_PIDIV4, 0, 0));
	camera->UpdateViewMatrix();
	camera->GetViewMatrix();
	ambientTerm = XMFLOAT3(0.0f, 0.0f, 0.0f); //sky red-ish to match sun peaking over planet

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
	{
		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		// Pick a style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Initialize helper Platform and Renderer backends (here we are using imgui_impl_win32.cpp and imgui_impl_dx11.cpp)
		ImGui_ImplWin32_Init(this->hWnd);
		ImGui_ImplDX11_Init(device.Get(), context.Get());
	}

	m_EntityManager = EntityManager::GetInstance();

	//get's the next multiple of 16, so that they'll be extra space
	unsigned int size = sizeof(VertexShaderData);
	size = (size + 15) / 16 * 16; //integer division to get rid of excess, * 16 to get byte size.

	LoadTextures();

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

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();

	//create the materials
	materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, pixelShader));//white material for medieval floor
	materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, pixelShader));//white material for scifi panel
	materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, pixelShader));//white material for cobblestone wall
	materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, pixelShader));//white material for bronze
	catapultMaterial = std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f, vertexShader, catapultPixelShader);

	//add textures to each material
	for (unsigned int i = 0; i < materials.size(); i++) {
		materials[i]->AddSampler("BasicSampler", basicSampler);
		materials[i]->AddTextureSRV("AlbedoTexture", albedoMaps[i]);
		materials[i]->AddTextureSRV("RoughnessTexture", roughnessMaps[i]);
		materials[i]->AddTextureSRV("AmbientTexture", aoMaps[i]);
		materials[i]->AddTextureSRV("NormalTexture", normalMaps[i]);
		materials[i]->AddTextureSRV("MetalnessTexture", metalnessMaps[i]);
	}

	catapultMaterial->AddSampler("BasicSampler", basicSampler);
	catapultMaterial->AddTextureSRV("AlbedoTexture", catapultMaps[0]);
	catapultMaterial->AddTextureSRV("RoughnessTexture", catapultMaps[1]);
	catapultMaterial->AddTextureSRV("NormalTexture", catapultMaps[2]);


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

void Game::LoadTextures() {
	//make sure there's space for CreateWICTextureFromFile
	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);
	//load textures
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Medieval_Floor_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);

	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/SciFi_Panel_Metalness.tif").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Brick_Wall_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/noMetal.png").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());

	albedoMaps.push_back(nullptr);
	roughnessMaps.push_back(nullptr);
	aoMaps.push_back(nullptr);
	normalMaps.push_back(nullptr);
	metalnessMaps.push_back(nullptr);
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Albedo.tif").c_str(), nullptr, albedoMaps[albedoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Roughness.tif").c_str(), nullptr, roughnessMaps[roughnessMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_AO.tif").c_str(), nullptr, aoMaps[aoMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Normal.tif").c_str(), nullptr, normalMaps[normalMaps.size() - 1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Bronze_Metallic.tif").c_str(), nullptr, metalnessMaps[metalnessMaps.size() - 1].GetAddressOf());


	//catapult materials
	catapultMaps.push_back(nullptr);
	catapultMaps.push_back(nullptr);
	catapultMaps.push_back(nullptr);

	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Catapult_Diffuse.tif").c_str(), nullptr, catapultMaps[0].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Catapult_Specular.tif").c_str(), nullptr, catapultMaps[1].GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Catapult_Bump.tif").c_str(), nullptr, catapultMaps[2].GetAddressOf());


	//load cube map
	skybox = CreateCubemap(
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_right.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_left.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_up.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_down.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_front.png").c_str(),
		GetFullPathTo_Wide(L"../../Assets/Textures/Sky/planet_back.png").c_str());

}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	//using Chirs's simple shader
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, GetFullPathTo_Wide(L"VertexShader.cso").c_str());
	skyVertexShader = std::make_shared<SimpleVertexShader>(device, context, GetFullPathTo_Wide(L"SkyVertexShader.cso").c_str());
	shadowVertexShader = std::make_shared<SimpleVertexShader>(device, context, GetFullPathTo_Wide(L"ShadowVertexShader.cso").c_str());

	pixelShader = std::make_shared<SimplePixelShader>(device, context, GetFullPathTo_Wide(L"PixelShader.cso").c_str());
	debugPixelShader = std::make_shared<SimplePixelShader>(device, context, GetFullPathTo_Wide(L"DebugColorShader.cso").c_str());
	skyPixelShader = std::make_shared<SimplePixelShader>(device, context, GetFullPathTo_Wide(L"SkyPixelShader.cso").c_str());
	shadowPixelShader = std::make_shared<SimplePixelShader>(device, context, GetFullPathTo_Wide(L"ShadowPixelShader.cso").c_str());

	catapultPixelShader = std::make_shared<SimplePixelShader>(device, context, GetFullPathTo_Wide(L"CatapultPixelShader.cso").c_str());
	a5PixelShader = std::make_shared<SimplePixelShader>(device, context, GetFullPathTo_Wide(L"A5CustomPS.cso").c_str());
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	//load object meshes
	meshes.push_back(std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/cube.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/cylinder.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/helix.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/sphere.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/quad.obj").c_str(), device, context));

	std::vector<Vertex> verts = meshes[3]->GetVerticies();

	XMFLOAT4 currPos = XMFLOAT4(verts[0].Position.x, verts[0].Position.y, verts[0].Position.z, 1.0f);

	//Inefficient could probs be better done through a compute shader
	float xMax = currPos.x;
	float xMin = currPos.x;

	float yMax = currPos.y;
	float yMin = currPos.y;

	float zMax = currPos.z;
	float zMin = currPos.z;
	for (int i = 1; i < verts.size(); i++)
	{
		currPos = XMFLOAT4(verts[i].Position.x, verts[i].Position.y, verts[i].Position.z, 1.0f);

		xMax = currPos.x > xMax ? currPos.x : xMax;
		xMin = currPos.x < xMin ? currPos.x : xMin;

		yMax = currPos.y > yMax ? currPos.y : yMax;
		yMin = currPos.y < yMin ? currPos.y : yMin;

		zMax = currPos.z > zMax ? currPos.z : zMax;
		zMin = currPos.z < zMin ? currPos.z : zMin;
	}

	Collider::SetDebugSphereMeshRadius(powf((xMax-xMin) / 2, 2) + powf((yMax - yMin) / 2, 2) + powf((zMax - zMin) / 2, 2));

	//meshes.push_back(std::make_shared<Mesh>(GetFullPathTo("../../Assets/Toon/tree obj.obj").c_str(), device, context));

	std::shared_ptr<Mesh> catapult = std::make_shared<Mesh>(GetFullPathTo("../../Assets/Models/catapult.obj").c_str(), device, context);

	//create some entities
	//cube direectly in front of camera
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[0], materials[0], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));
	//sphere to left of cube
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[3], materials[3], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));
	//helix to right
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[2], materials[3], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));
	//helix below cube
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[2], materials[2], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));
	//cylinder one behind cube
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[1], materials[1], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));
	//cylinder above cube
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[1], materials[3], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));

	m_EntityManager->GetEntity(2)->GetTransform()->AddChild(m_EntityManager->GetEntity(5)->GetTransform());

	//big plane to act as floor
	m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[4], materials[2], camera, std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f), 0.5f, vertexShader, debugPixelShader), camera), device));

	//move objects so there isn't overlap
	m_EntityManager->GetEntity(0)->GetTransform()->MoveAbsolute(XMFLOAT3(-2.5f, 0, 2.5f));
	m_EntityManager->GetEntity(1)->GetTransform()->MoveAbsolute(XMFLOAT3(5.0f, 10.0f, 5.0f));
	m_EntityManager->GetEntity(2)->GetTransform()->MoveAbsolute(XMFLOAT3(7.0f, 3.0f, 3.0f));
	m_EntityManager->GetEntity(3)->GetTransform()->MoveAbsolute(XMFLOAT3(0.0f, 0.0f, -5.0f));
	m_EntityManager->GetEntity(4)->GetTransform()->MoveAbsolute(XMFLOAT3(4.0f, 0.0f, -4.0f));
	m_EntityManager->GetEntity(5)->GetTransform()->MoveAbsolute(XMFLOAT3(2.5f, 0.0f, -2.5f));
	m_EntityManager->GetEntity(6)->GetTransform()->MoveAbsolute(XMFLOAT3(0.0f, 0.0f, 5.0f)); //move left and down
	m_EntityManager->GetEntity(6)->GetTransform()->Rotate(XMFLOAT3(-1 * XM_PIDIV2, 0, 0));
	m_EntityManager->GetEntity(6)->GetTransform()->Scale(20);//scale up a bunch to act as floor

	//catapult
	if (catapult->GetVertexBuffer()) {
		meshes.push_back(catapult);
		m_EntityManager->AddEntity(std::make_shared<GameEntity>(meshes[5], catapultMaterial, camera));
		m_EntityManager->GetEntity(7)->GetTransform()->MoveAbsolute(XMFLOAT3(-2.5f, -5.0f, -2.5f));
		m_EntityManager->GetEntity(7)->GetTransform()->SetScale(XMFLOAT3(0.25f, 0.25f, 0.25f));
	}

	//create sky obj
	sky = std::make_shared<Sky>(meshes[0], basicSampler, skybox, device, context, skyVertexShader, skyPixelShader);
}


void Game::CreateLights() {
	XMFLOAT3 white = { 1,1,1 };
	//create direction light
	Light temp = {};
	temp.Type = LIGHT_TYPE_DIRECTIONAL;
	temp.Direction = XMFLOAT3(1, -2, 0); // point directly 'right'
	temp.Color = white;//XMFLOAT3(0, 0, 1);//bright blue 
	temp.Intensity = 0.005f; //each for testing right now
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
	Microsoft::WRL::ComPtr<ID3D11Texture2D> cubeMapTexture = 0;
	device->CreateTexture2D(&cubeDesc, 0, cubeMapTexture.GetAddressOf());


	for (int i = 0; i < 6; i++) {
		//create depth/stencil
		D3D11_DEPTH_STENCIL_VIEW_DESC shadowBoxStencilDesc = {};
		shadowBoxStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		shadowBoxStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		shadowBoxStencilDesc.Texture2DArray.MipSlice = 0;
		shadowBoxStencilDesc.Texture2DArray.ArraySize = 1;
		shadowBoxStencilDesc.Texture2DArray.FirstArraySlice = i;

		shadowBoxStencils.push_back(nullptr);
		device->CreateDepthStencilView(cubeMapTexture.Get(), &shadowBoxStencilDesc, shadowBoxStencils[shadowBoxStencils.size() - 1].GetAddressOf());
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
	srvDesc.TextureCube.MipLevels = 1; // Only need access to 1 mip
	srvDesc.TextureCube.MostDetailedMip = 0; // Index of the first mip we want to see
	device->CreateShaderResourceView(cubeMapTexture.Get(), &srvDesc, shadowBoxSRV.GetAddressOf());


}

void Game::RenderDirectionalShadowMap()
{

	//create "camera" mats
	XMMATRIX shView = XMMatrixLookAtLH(
		XMVectorSet(-20, 40, 0, 0),//move 20 units left to 'match' lig
		XMVectorSet(0, 0, 0, 0),
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
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	vp.Width = (float)this->width;
	vp.Height = (float)this->height;
	context->RSSetViewports(1, &vp);
	context->RSSetState(0); //reset
}

void Game::RenderPointShadowMap(DirectX::XMFLOAT3 pos, float range, float nearZ, float farZ)
{

	//unbind shadow resource
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	context->PSSetShaderResources(6, 1, pSRV);

	//setup pipline for shadow map
	context->OMSetRenderTargets(0, 0, shadowBoxStencils[0].Get());
	context->ClearDepthStencilView(shadowBoxStencils[0].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
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

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[1].Get());
	context->ClearDepthStencilView(shadowBoxStencils[1].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

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

#pragma region Up

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[2].Get());
	context->ClearDepthStencilView(shadowBoxStencils[2].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

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

#pragma region Down

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[3].Get());
	context->ClearDepthStencilView(shadowBoxStencils[3].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

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

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[4].Get());
	context->ClearDepthStencilView(shadowBoxStencils[4].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

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

#pragma region Back

	context->OMSetRenderTargets(0, 0, shadowBoxStencils[5].Get());
	context->ClearDepthStencilView(shadowBoxStencils[5].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

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

	//reset all render states
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	vp.Width = (float)this->width;
	vp.Height = (float)this->height;
	context->RSSetViewports(1, &vp);
	context->RSSetState(0); //reset
}

void Game::RenderSpotShadowMap(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, float range, float spotFallOff, float nearZ, float farZ)
{

	//unbind shadow resource
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	context->PSSetShaderResources(7, 1, pSRV); //spot shadow map is 7th

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
	XMMATRIX shProj = XMMatrixPerspectiveFovLH(45.0f * toRadians, 1.0f, nearZ, farZ);// XMMatrixPerspectiveLH(spotFallOff, spotFallOff, nearZ, farZ);
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
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
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

			//Lambda function cause I thought it'd be cool but as it turns out it was more of a hassle than it was worth lol
			static auto addEntity = [&](auto&& addEntity, Transform* entityTransform, int entityNum) {

				ImGui::PushID(entityTransform);
				bool nodeOpen = ImGui::TreeNode("Entity", "%s %i", "Entity", entityNum);
				if (!nodeOpen) {
					ImGui::PopID();
					return;
				}

				//Allows for control over position of entities
				DirectX::XMFLOAT3 position = entityTransform->GetPosition();
				ImGui::Text("Position: ");
				ImGui::SameLine();
				ImGui::DragFloat3("", &position.x, .5f, -D3D11_FLOAT32_MAX, D3D11_FLOAT32_MAX);
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
				for (int i = 0; i < numChildren; i++)
				{
					addEntity(addEntity, entityTransform->GetChild(i), i + 1);
				}

				ImGui::TreePop();
				ImGui::PopID();
			};

			int entityID = 1;
			for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
				std::shared_ptr<GameEntity> currEntity = m_EntityManager->GetEntity(i);

				if (currEntity->GetTransform()->GetParent())
					continue;

				addEntity(addEntity, currEntity->GetTransform(), entityID);
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

		if (camera) {
			ImGui::Text("Move Speed: ");
			ImGui::SameLine();
			ImGui::DragFloat(" ", camera->GetMoveSpeed(), .01f, 0.01f, 10.0f);

			ImGui::Text("Mouse Move Speed: ");
			ImGui::SameLine();
			ImGui::DragFloat("  ", camera->GetMouseMoveSpeed(), .01f, 0.01f, 2.0f);
		}

		ImGui::PopID();

		// Show the demo window
		//ImGui::ShowDemoWindow();
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE)) {
		Quit();
	}

	//make objects move, scale, or rotate
	m_EntityManager->GetEntity(0)->GetTransform()->MoveRelative(XMFLOAT3(-sin(totalTime * 2.0f) * 0.25f, 0, 0));
	m_EntityManager->GetEntity(1)->GetTransform()->MoveRelative(XMFLOAT3(0, sin(totalTime) * 0.25f, 0));

	m_EntityManager->GetEntity(2)->GetTransform()->Rotate(XMFLOAT3(0, 0, deltaTime * 0.5f));
	m_EntityManager->GetEntity(3)->GetTransform()->Rotate(XMFLOAT3(0, deltaTime * 0.5f, 0));
	m_EntityManager->GetEntity(4)->GetTransform()->Rotate(XMFLOAT3(deltaTime * 0.5f, 0, 0));

	m_EntityManager->UpdateEntities(deltaTime);

	CreateGui(deltaTime);

	camera->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
	//stuff above needed every frame.

	//do shadow rendering stuff
	for (int i = 0; i < lights.size(); i++) {
		if (lights[i].CastsShadows) {
			if (lights[i].Type == LIGHT_TYPE_DIRECTIONAL) {
				RenderDirectionalShadowMap();
			}
			else if (lights[i].Type == LIGHT_TYPE_POINT) {
				RenderPointShadowMap(lights[i].Position, lights[i].Range, lights[i].NearZ, lights[i].FarZ);
			}
			else if (lights[i].Type == LIGHT_TYPE_SPOT) {
				RenderSpotShadowMap(lights[i].Position, lights[i].Direction, lights[i].Range, lights[i].SpotFalloff, lights[i].NearZ, lights[i].FarZ);
			}
		}
	}

	//draw game entities
	for (int i = 0; i < m_EntityManager->NumEntities(); i++) {
		std::shared_ptr<GameEntity> entity = m_EntityManager->GetEntity(i);

		std::shared_ptr<SimpleVertexShader> vs = entity->GetMaterial()->GetVertexShader();
		//send shadow info to vertex shader
		vs->SetMatrix4x4("lightView", shadowViewMat);
		vs->SetMatrix4x4("lightProj", shadowProjMat);
		vs->SetMatrix4x4("spotLightView", spotShadowViewMat);
		vs->SetMatrix4x4("spotLightProj", spotShadowProjMat);
		for (int j = 0; j < lights.size(); j++) {
			if (lights[j].Type == LIGHT_TYPE_POINT)
				vs->SetFloat3("lightPos", lights[j].Position);
		}

		std::shared_ptr<SimplePixelShader> ps = entity->GetMaterial()->GetPixelShader();
		//send light data to shaders
		ps->SetInt("numLights", static_cast<int>(lights.size()));
		ps->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		ps->SetShaderResourceView("ShadowMap", shadowSRV);
		ps->SetShaderResourceView("ShadowBox", shadowBoxSRV);
		ps->SetShaderResourceView("ShadowSpotMap", shadowSpotSRV);

		ps->SetFloat3("ambientTerm", ambientTerm);
		ps->SetSamplerState("ShadowSampler", shadowSampler);
		entity->GetMaterial()->PrepareMaterial();

		entity->Draw();
	}

	//draw sky, after everthying else to reduce overdraw
	sky->Draw(camera);

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
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
}


//helper function to create cube map from 6 textures.
//used with permission from Chris Cascioli
// --------------------------------------------------------
// Loads six individual textures (the six faces of a cube map), then
// creates a blank cube map and copies each of the six textures to
// another face. Afterwards, creates a shader resource view for
// the cube map and cleans up all of the temporary resources.
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Game::CreateCubemap(
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
	CreateWICTextureFromFile(device.Get(), right, (ID3D11Resource**)&textures[0], 0);
	CreateWICTextureFromFile(device.Get(), left, (ID3D11Resource**)&textures[1], 0);
	CreateWICTextureFromFile(device.Get(), up, (ID3D11Resource**)&textures[2], 0);
	CreateWICTextureFromFile(device.Get(), down, (ID3D11Resource**)&textures[3], 0);
	CreateWICTextureFromFile(device.Get(), front, (ID3D11Resource**)&textures[4], 0);
	CreateWICTextureFromFile(device.Get(), back, (ID3D11Resource**)&textures[5], 0);
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
	device->CreateTexture2D(&cubeDesc, 0, &cubeMapTexture);
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
			context->CopySubresourceRegion(
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
		device->CreateShaderResourceView(cubeMapTexture, &srvDesc, cubeSRV.GetAddressOf());
	}
	// Now that we're done, clean up the stuff we don't need anymore
	cubeMapTexture->Release(); // Done with this particular reference (the SRV has another)
	for (int i = 0; i < 6; i++)
		textures[i]->Release();
	// Send back the SRV, which is what we need for our shaders
	return cubeSRV;
}