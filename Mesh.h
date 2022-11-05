//Author: Cassiar Beaver
//Represents a mesh using an index buffer
//Handles drawing mesh to screen
#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

#include "Vertex.h"
#include <Saba/Model/MMD/PMXModel.h>

class Mesh
{
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertBuf;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuf;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	unsigned int numIndices;

	std::vector<Vertex> m_verts;

	std::shared_ptr<saba::PMXModel> model;
	bool isPmx;

	void CreateBuffers(Vertex* in_verts, unsigned int numVerts, unsigned int * in_indices, Microsoft::WRL::ComPtr<ID3D11Device> device);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

public:
	//create a mesh by passing in the verts and indices lists
	Mesh(Vertex * in_verts, unsigned int numVerts, unsigned int * in_indices, unsigned int in_numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context);
	//load a mesh by passing in the name of a file
	Mesh(const char* path, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context);
	//load a pmx mesh using the saba library
	Mesh(const char* path, const char* texpath, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context);
	~Mesh();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	std::vector<Vertex> GetVerticies() { return m_verts; }
	unsigned int GetIndexCount();
	void Draw();
	void Draw(Microsoft::WRL::ComPtr<ID3D11RasterizerState> customRast);
};
