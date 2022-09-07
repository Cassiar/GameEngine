//Author: Cassiar Beaver
//Represents a mesh using an index buffer
//Handles drawing mesh to screen
#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects


#include "Vertex.h"

class Mesh
{
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertBuf;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuf;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	unsigned int numIndices;

	void CreateBuffers(Vertex* in_verts, unsigned int numVerts, unsigned int * in_indices, Microsoft::WRL::ComPtr<ID3D11Device> device);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

public:
	//create a mesh by passing in the verts and indices lists
	Mesh(Vertex * in_verts, unsigned int numVerts, unsigned int * in_indices, unsigned int in_numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context);
	//load a mesh by passing in the name of a file
	Mesh(const char* path, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context);
	~Mesh();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	unsigned int GetIndexCount();
	void Draw();
};
