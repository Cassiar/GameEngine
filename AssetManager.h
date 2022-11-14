#pragma once

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "SabaMesh.h"
#include "SimpleShader.h"

#include <fstream>
#include <map>
#include <WICTextureLoader.h>

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

// mmd shader constant buffer

//========================
//Saba Code
//========================
#define	STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb/stb_image.h>

class AssetManager
{
private:
	const std::wstring TEXTURE_FOLDER = L"../../Assets/Textures/";
	const std::string MODEL_FOLDER = "../../Assets/Models/";

	static std::shared_ptr<AssetManager> s_instance;

	Microsoft::WRL::ComPtr<ID3D11Device> m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;

	std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>> m_vertexShaders;
	std::unordered_map<std::string, std::string> m_vertShaderCsoNames;

	std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>> m_pixelShaders;
	std::unordered_map<std::string, std::string> m_pixelShaderCsoNames;

	std::unordered_map<SRVMaps, std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>> m_srvMaps;
	std::unordered_map<SRVMaps, std::vector<std::string>> m_srvFileNames;

	std::map<std::string, SabaTexture>	m_sabaTextures;

	//Needs to be either deleted or moved into m_PixelShaders
	//std::shared_ptr<SimplePixelShader> watercolorPixelShader;

	//array to hold materials
	std::vector<std::shared_ptr<Material>>		m_materials;
	std::vector<std::shared_ptr<Material>>		m_toonMaterials;
	std::vector<std::shared_ptr<Material>>		m_sabaMaterials;
	std::vector<SabaMaterial>					m_sabaStructMaterials;
	std::vector<std::shared_ptr<Material>>		m_sabaEdgeMaterials;

	//array to hold meshes
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	std::vector<std::shared_ptr<SabaMesh>> m_sabaMeshes;
	std::vector<std::shared_ptr<Mesh>> m_toonMeshes;

	// Not sure these belong here
	//std::shared_ptr<Camera> camera;
	//Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> m_samplers;

	AssetManager(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void Init(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void InitTextures();
	void InitShaders();
	void InitMeshes();
	void InitSamplers();
	void InitMaterials();
	bool InitSaba(std::shared_ptr<SabaMesh> mesh);
	bool InitSabaShaders(std::shared_ptr<SabaMesh> mesh);
	void InitSabaMaterials(std::shared_ptr<SabaMesh> mesh);
	SabaTexture GetTexture(const std::string& texturePath);

public:
	// nshields TODO - Shouldn't be public but for simplicity of deadline i'm breaking rules
#pragma region SabaStuff

	Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_textureSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_toonTextureSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_sphereTextureSampler;
	Microsoft::WRL::ComPtr<ID3D11BlendState>	m_mmdBlendState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdFrontFaceRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdBothFaceRS;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_mmdEdgeInputLayout;
	Microsoft::WRL::ComPtr<ID3D11BlendState>	m_mmdEdgeBlendState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_mmdEdgeRS;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_defaultDSS;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_dummyTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_dummyTextureView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_dummySampler;
#pragma endregion

	// Normally this intialization function wouldn't be necessary, however, for this
	// Singleton to make sense we need access to the ID3D11Device and ID3D11DeviceContext.
	// I don't want the user to have to pass these through in the GetInstance function
	// everytime so instead this Singleton needs to be initialized before GetInstance
	// can be called.
	static void InitializeSingleton(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	static std::shared_ptr<AssetManager> GetInstance();

	~AssetManager();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV(SRVMaps map, int srvIndex);

	std::shared_ptr<Mesh> GetMesh(int index) { return m_meshes[index]; }
	std::vector<std::shared_ptr<Mesh>> GetMeshes() { return m_meshes; }

	std::shared_ptr<Mesh> GetToonMesh(int index) { return m_toonMeshes[index]; }
	std::vector<std::shared_ptr<Mesh>> GetToonMeshes() { return m_toonMeshes; }
	
	std::shared_ptr<SabaMesh> GetSabaMesh(int index) { return m_sabaMeshes[index]; }
	std::vector<std::shared_ptr<SabaMesh>> GetSabaMeshes() { return m_sabaMeshes; }

	std::shared_ptr<Material> GetMaterial(int index) { return m_materials[index]; }
	std::vector<std::shared_ptr<Material>> GetMaterials() { return m_materials; }
	std::vector<std::shared_ptr<Material>> GetSabaMaterials() { return m_sabaMaterials; }
	std::vector<SabaMaterial> GetSabaStructMaterials() { return m_sabaStructMaterials; }

	std::shared_ptr<Material> GetToonMaterial(int index) { return m_toonMaterials[index]; }
	std::vector<std::shared_ptr<Material>> GetToonMaterials() { return m_toonMaterials; }

	std::shared_ptr<SimpleVertexShader> GetVertexShader(std::string shaderName) { return m_vertexShaders[shaderName]; }
	std::shared_ptr<SimplePixelShader> GetPixelShader(std::string shaderName) { return m_pixelShaders[shaderName]; }

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSampler(std::string samplerName) { return m_samplers[samplerName]; }

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

	//void AddSRVToMap(SRVMaps mapTypeName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvToAdd);
	void AddSRVToMap(SRVMaps mapTypeName, std::wstring srvPath, bool customPath = false);

	void AddVertShaderToMap(std::string key, std::string filename);
	std::shared_ptr<SimpleVertexShader> MakeSimpleVertexShader(std::wstring csoName);

	void AddPixelShaderToMap(std::string key, std::string filename);
	std::shared_ptr<SimplePixelShader> MakeSimplePixelShader(std::wstring csoName);

	void AddSabaMesh(std::shared_ptr<SabaMesh> mesh) { m_sabaMeshes.push_back(mesh); }

	void MakeRasterizerState(D3D11_RASTERIZER_DESC rastDesc, Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rastLocation);

	std::shared_ptr<Material> ReadMaterialFromFile(std::wstring path);

	///------------------ Written by Chris Cascioli ------------------------------///
	// Helpers for determining the actual path to the executable
	std::string GetExePath();
	std::wstring GetExePath_Wide();

	//Max string length to convert is 1024
	std::wstring StringToWide(std::string str);
	std::string WideToString(std::wstring str);

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);
	///---------------------------------------------------------------------------///

};