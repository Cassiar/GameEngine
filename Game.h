#pragma once

#include "AssetManager.h"
#include "Camera.h"
#include "DXCore.h"
#include "EntityManager.h"
#include "GameEntity.h"
#include "Lights.h"
#include "Material.h"
#include "Mesh.h"
#include "SimpleShader.h"
#include "Sky.h"
#include "Transform.h"

#include <DirectXMath.h>
#include <fstream>
#include <iostream>
#include <memory> //for shared pointers
#include <unordered_map> //to avoid searching in ImGui debug sphere drawing
#include <vector> //for vector
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <Saba/Model/MMD/VMDFile.h>

#define IMGUI_USE_WCHAR32

//handles updating game logic and stores objects that are drawn
class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:
	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	// Should we use vsync to limit the frame rate?
	bool vsync;
	const float toRadians = 3.1415f / 180.0f;

	// Initialization helper methods - feel free to customize, combine, etc.
	void CreateBasicGeometry();
	void CreateLights();
	void CreateShadowResources();
	void CreateExtraRenderTargets();
	void CreateGui(float deltaTime);

	void CreateMaterialGUI(float deltaTime);

	//void CreateMaterialGUI();

	void RenderDirectionalShadowMap(DirectX::XMFLOAT3 dir, DirectX::XMFLOAT3 targetPos);
	void RenderPointShadowMap(DirectX::XMFLOAT3 pos, int index, float range, float nearZ, float farZ);
	void RenderSpotShadowMap(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, float range, float spotFallOff, float nearZ, float farZ);
	
	//helper function to reduce repitition in shadow map funcs
	//void PassShadowObjs();

	void Save() override;

	void WriteToFile(std::string name, std::string writeString);

	void SaveScene();
	void ReadScene(std::wstring path);

	DirectX::XMFLOAT3 ambientTerm;

	//test light for lighting equations
	std::vector<Light> lights;
	//array to hold each light's position, used to simplify passing in point light info
	std::vector<DirectX::XMFLOAT3> lightPoses;

	//ImFont* font;

	//simple shader stuff
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;


	std::shared_ptr<SimplePixelShader> toonPixelShader;
	std::shared_ptr<SimplePixelShader> debugPixelShader;
	std::shared_ptr<SimpleVertexShader> skyVertexShader;
	std::shared_ptr<SimplePixelShader> skyPixelShader;
	
	std::shared_ptr<SimpleVertexShader> shadowVertexShader;
	std::shared_ptr<SimplePixelShader> shadowPixelShader;

	std::shared_ptr<SimpleVertexShader> ppLightRaysVertexShader;
	std::shared_ptr<SimplePixelShader> ppLightRaysPixelShader;

	std::shared_ptr<SimplePixelShader> a5PixelShader;
	std::shared_ptr<SimplePixelShader> watercolorPixelShader;	
	
	//copies of saba shaders
	std::shared_ptr<SimpleVertexShader> mmdVertexShader;
	std::shared_ptr<SimplePixelShader> mmdPixelShader;
	std::shared_ptr<SimpleVertexShader> mmdEdgeVertexShader;
	std::shared_ptr<SimplePixelShader> mmdEdgePixelShader;

	//array to hold meshes
	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<Mesh>> toonMeshes;
	//array to hold game entities
	std::vector<std::shared_ptr<GameEntity>> renderableEntities;

	std::shared_ptr<EntityManager> m_EntityManager;
	std::shared_ptr<AssetManager> m_AssetManager;
	std::shared_ptr<PhysXManager> m_PhysicsManager;

	std::shared_ptr<Camera> camera;

	//pointer for sky box
	std::shared_ptr<Sky> sky;

	//shadow mapping stuff
	int shadowResolution; // size of texture to send to must be power of 2
	float shadowProjSize; //size of world that it can see
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> shadowBoxSRVs;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSpotSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowSpotStencil;
	std::vector<std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>>> shadowBoxStencils;

	//need custom samplers and rasterizers
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	DirectX::XMFLOAT4X4 shadowViewMat;
	DirectX::XMFLOAT4X4 shadowProjMat;

	DirectX::XMFLOAT4X4 spotShadowViewMat;
	DirectX::XMFLOAT4X4 spotShadowProjMat;

	//variables to control light rays effect
	float lightRaysDensity = 0.25f;
	float lightRaysWeight = 0.2f;
	float lightRaysDecay = 0.98f;
	float lightRaysExposure = 0.2f;
	bool enableLightRays = false;

	std::shared_ptr<Mesh> sabaLisa;
	std::shared_ptr<GameEntity> sabaEntity;
	std::shared_ptr<saba::VMDFile> animFile;
	std::shared_ptr<saba::VMDAnimation> anim;
	float animTime = 0;
	double saveTime = 0;

	bool animOn = false;
	bool runAnim = false;
	bool morphAnim = false;

	std::shared_ptr<std::vector<float>> morphWeights;
};

