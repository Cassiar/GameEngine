//Author: Cassiar Beaver
//A class to define the surface appearance of a mesh.
//can be shared between game entities. 
#pragma once

#include "Serializable.h"
#include "StringSerializable.h"
#include "SRVMapsEnum.h"

#include <fstream>
#include <iostream>
#include <memory> //for shared pointers
#include <unordered_map> //for hash tables (basically dictionaries)

#include "SimpleShader.h"

#define MATERIAL_MAX_SERIAL_SRVS 16
#define MATERIAL_MAX_SERIAL_SAMPLERS 8

#pragma pack(push, 1)
struct MaterialSerialData : SerialData {
	DirectX::XMFLOAT4 colorTint;
	float roughness;

	SRVMaps			srvTypes[MATERIAL_MAX_SERIAL_SRVS];
	std::string		srvNames[MATERIAL_MAX_SERIAL_SRVS];
	std::string		srvFileNames[MATERIAL_MAX_SERIAL_SRVS];

	std::string	vsFileName;
	std::string		vsName;
	std::string	psFileName;
	std::string		psName;

	std::string		samplerNames[MATERIAL_MAX_SERIAL_SAMPLERS];
};
#pragma pack(pop)

class Material : Serializable, StringSerializable
{
public:
	//create a materal. Must have color tint vertex and pixel shaders
	Material(DirectX::XMFLOAT4 in_color, float roughness, std::shared_ptr<SimpleVertexShader> in_vs, std::shared_ptr<SimplePixelShader> in_ps);
	~Material();

	//get a copy of the color tint
	DirectX::XMFLOAT4 GetColorTint();
	//set the color tine
	void SetColorTint(DirectX::XMFLOAT4 in_color);

	//get a copy of the roughness value
	float GetRoughness();
	//change roughness value
	void SetRoughness(float amount);

	//get a reference to the simple vertex shader
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> in_vs, std::string filename, std::string name);
	
	//get a reference to the simple pixel shader
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	void SetPixelShader(std::shared_ptr<SimplePixelShader> in_ps, std::string filename, std::string name);

	//add a texture SRV, name = name in hash table, srv = actual srv
	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, std::string fileName, SRVMaps srvType);
	//add a sampler state, name = name in hash table, sampler = actual sampler
	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);

	//prepare a material for drawing. Sets up any material only items (e.g. srvs)
	void PrepareMaterial();

	std::string SerializeToString();

	void WriteToBinary(std::wstring filePath) override;
	SerialData ReadBinary(std::wstring filePath) override;
	
private:
	DirectX::XMFLOAT4 colorTint;
	//value between 0 and 1, 1 'perfectly' matte, 0 'perflectly' reflective.
	float roughness;
	std::shared_ptr<SimpleVertexShader> vs;
	std::string vsFileName;
	std::string vsName;

	std::shared_ptr<SimplePixelShader> ps;
	std::string psFileName;
	std::string psName;

	//storing the textures and samplers for a material
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, std::string> textureFiles;
	std::unordered_map<std::string, SRVMaps> textureTypes;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
};

