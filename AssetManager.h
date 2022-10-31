#pragma once

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "SimpleShader.h"

#include "WICTextureLoader.h"

enum SRVMaps
{
	Albedo,
	Roughness,
	AO,
	Normal,
	Metalness,
	ToonAlbedo,
	ToonRoughness,
	ToonAO,
	ToonMetalness,
	SampleTexture,
	SkyBox
};

class AssetManager
{
private:
	const std::wstring TEXTURE_FOLDER = L"../../Assets/Textures/";
	const std::string MODEL_FOLDER = "../../Assets/Models/";

	static std::shared_ptr<AssetManager> s_instance;

	Microsoft::WRL::ComPtr<ID3D11Device> m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;

	std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>> m_vertexShaders;
	std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>> m_pixelShaders;

	std::unordered_map<SRVMaps, std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>> m_srvMaps;

	//Needs to be either deleted or moved into m_PixelShaders
	//std::shared_ptr<SimplePixelShader> watercolorPixelShader;

	//array to hold materials
	std::vector<std::shared_ptr<Material>> m_materials;
	std::vector<std::shared_ptr<Material>> m_toonMaterials;

	//array to hold meshes
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	std::vector<std::shared_ptr<Mesh>> m_toonMeshes;

	// Not sure these belong here
	//std::shared_ptr<Camera> camera;
	//Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler;

	AssetManager();
	void Init(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void InitTextures();
	void InitShaders();
	void InitMeshes();

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

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadSRV(std::wstring texturePath, bool customLocation = false);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadSRV(std::wstring texturePath, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureDestination, bool customLocation = false);

	std::shared_ptr<Mesh> LoadMesh(std::string meshPath, bool customLocation = false);

	void AddSRVToMap(SRVMaps mapTypeName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvToAdd);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV(SRVMaps map, int srvIndex);

	std::shared_ptr<SimpleVertexShader> MakeSimpleVertexShader(std::wstring csoName);
	std::shared_ptr<SimplePixelShader> MakeSimplePixelShader(std::wstring csoName);


	///------------------ Written by Chris Cascioli ------------------------------///
	// Helpers for determining the actual path to the executable
	std::string GetExePath();
	std::wstring GetExePath_Wide();

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);
	///---------------------------------------------------------------------------///

};