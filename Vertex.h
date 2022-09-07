#pragma once

#include <DirectXMath.h>

// --------------------------------------------------------
// A custom vertex definition
//
// You will eventually ADD TO this, and/or make more of these!
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The local position of the vertex
	DirectX::XMFLOAT3 Normal;	//normal vector
	DirectX::XMFLOAT3 Tangent;
	//DirectX::XMFLOAT3 Bitangent;
	DirectX::XMFLOAT2 UVCoord;	//uv coordinate
};