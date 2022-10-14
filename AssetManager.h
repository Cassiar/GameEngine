#pragma once

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "SimpleShader.h"

class AssetManager
{
private:
	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

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

	std::shared_ptr<Camera> camera;

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

	void Init();

public:
	AssetManager();
	~AssetManager();

};