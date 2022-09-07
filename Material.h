//Author: Cassiar Beaver
//A class to define the surface appearance of a mesh.
//can be shared between game entities. 
#pragma once

#include <memory> //for shared pointers
#include <unordered_map> //for hash tables (basically dictionaries)

#include "SimpleShader.h"
class Material
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
	//change the vertex shader
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> in_vs);
	
	//get a reference to the simple pixel shader
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	//change the pixel shader
	void SetPixelShader(std::shared_ptr<SimplePixelShader> in_ps);

	//add a texture SRV, name = name in hash table, srv = actual srv
	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	//add a sampler state, name = name in hash table, sampler = actual sampler
	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);

	//prepare a material for drawing. Sets up any material only items (e.g. srvs)
	void PrepareMaterial();
private:
	DirectX::XMFLOAT4 colorTint;
	//value between 0 and 1, 1 'perfectly' matte, 0 'perflectly' reflective.
	float roughness;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;

	//storing the textures and samplers for a material
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
};

