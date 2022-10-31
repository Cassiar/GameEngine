#pragma once

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "SimpleShader.h"

#include "WICTextureLoader.h"

enum SRVMaps
{
	Albedo,
	AO,
	Normal,
	Metalness,
	ToonAlbedo,
	ToonAO,
	ToonMetalness
};

class AssetManager
{
private:
	static std::shared_ptr<AssetManager> s_instance;

	Microsoft::WRL::ComPtr<ID3D11Device> m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

	std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>> m_vertexShaders;
	std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>> m_pixelShaders;

	std::unordered_map<SRVMaps, std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>> m_srvMaps;


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
	std::vector<std::shared_ptr<Material>> m_materials;

	std::vector<std::shared_ptr<Material>> m_toonMaterials;

	//array to hold meshes
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	std::vector<std::shared_ptr<Mesh>> m_toonMeshes;

	std::shared_ptr<Camera> camera;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rampTexture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler;

	//index 0 is diffuse, 1 is specular, 2 is bump
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> catapultMaps;
	std::shared_ptr<Material> catapultMaterial;
	std::shared_ptr<SimplePixelShader> catapultPixelShader;

	AssetManager();
	void Init(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void InitTextures();

public:
	// Normally this intialization function wouldn't be necessary, however, for this
	// Singleton to make sense we need access to the ID3D11Device and ID3D11DeviceContext.
	// I don't want the user to have to pass these through in the GetInstance function
	// everytime so instead this Singleton needs to be initialized before GetInstance
	// can be called.
	static void InitializeSingleton(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	static std::shared_ptr<AssetManager> GetInstance();

	~AssetManager();

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadSRV(const wchar_t* texturePath);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadSRV(const wchar_t* texturePath, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureDestination);

	void AddSRVToMap(SRVMaps mapTypeName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvToAdd);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV(SRVMaps map, int srvIndex);

	///------------------ Written by Chris Cascioli ------------------------------///
	// Helpers for determining the actual path to the executable
	std::string GetExePath();
	std::wstring GetExePath_Wide();

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);
	///---------------------------------------------------------------------------///

};