#pragma once

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
#include <memory> //for shared pointers
#include <unordered_map> //to avoid searching in ImGui debug sphere drawing
#include <vector> //for vector
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <Saba/Model/MMD/VMDFile.h>

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
	void LoadTextures();
	void LoadShaders(); 
	void CreateBasicGeometry();
	void CreateLights();
	void CreateShadowResources();
	void CreateExtraRenderTargets();
	void CreateGui(float deltaTime);

	void RenderDirectionalShadowMap(DirectX::XMFLOAT3 dir);
	void RenderPointShadowMap(DirectX::XMFLOAT3 pos, int index, float range, float nearZ, float farZ);
	void RenderSpotShadowMap(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, float range, float spotFallOff, float nearZ, float farZ);
	
	//helper function to reduce repitition in shadow map funcs
	void PassShadowObjs();

	DirectX::XMFLOAT3 ambientTerm;

	//test light for lighting equations
	std::vector<Light> lights;
	//array to hold each light's position, used to simplify passing in point light info
	std::vector<DirectX::XMFLOAT3> lightPoses;

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//    Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Shaders and shader-related constructs
	// now handled by simple shader
	//Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	//Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	//Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

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

	//array to hold materials
	std::vector<std::shared_ptr<Material>> materials;

	std::vector<std::shared_ptr<Material>> toonMaterials;

	//array to hold meshes
	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<Mesh>> toonMeshes;
	//array to hold game entities
	std::vector<std::shared_ptr<GameEntity>> renderableEntities;

	std::shared_ptr<EntityManager> m_EntityManager;

	std::shared_ptr<Camera> camera;

	//pointer for sky box
	std::shared_ptr<Sky> sky;

	//texture related stuff
	//create srv for medieval floor albedo
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> albedoMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> roughnessMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> aoMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> normalMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> metalnessMaps;

	//toon maps
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> toonAlbedoMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> toonRoughnessMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> toonAoMaps;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> toonMetalnessMaps;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rampTexture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler;

	//index 0 is diffuse, 1 is specular, 2 is bump
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> catapultMaps;
	std::shared_ptr<Material> catapultMaterial;
	std::shared_ptr<SimplePixelShader> catapultPixelShader;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skybox;

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

	//testing stuff for post process
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppLightRaysSampler;

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
};

