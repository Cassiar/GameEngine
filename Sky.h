#pragma once
//@author: Cassiar Beaver cdb7951
//a class to represent a sky box in a game engine

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory> //for shared pointers

#include "SimpleShader.h"
#include "Mesh.h"
#include "Camera.h"

class Sky
{
private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<SimplePixelShader> ps;
	std::shared_ptr<SimpleVertexShader> vs;

	//stored so we don't have to pass in during draw
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
public:
	/// <summary>
	/// Creates a cube box to act as the sky. 
	/// Creates the raserizer and depth stencil state within the constructor
	/// </summary>
	/// <param name="mesh">Mesh to act as the sky, should be a cube</param>
	/// <param name="sampler">sampler state to get the texture</param>
	/// <param name="device">Needed to create the states</param>
	/// <param name="srv">Pointer to cube texture</param>
	Sky(std::shared_ptr<Mesh> mesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<SimplePixelShader> ps);
	~Sky();

	void Draw(std::shared_ptr<Camera> camera);
};

