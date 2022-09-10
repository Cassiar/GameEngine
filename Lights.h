#pragma once
#include <DirectXMath.h>

//major light types
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

//define struct to hold needed data for our lights
struct Light {
	int Type; //which kind of light, use those above
	DirectX::XMFLOAT3 Direction; //direction and spot lights need to know where they're pointing
	float Range; //spot and point have max range
	DirectX::XMFLOAT3 Position; //spot and point need to kow location in 3d space.
	float Intensity; //All need this
	DirectX::XMFLOAT3 Color; //all lights need a color
	float SpotFalloff; //spot lights need to have cone size
	bool CastsShadows;
	float NearZ; //near and far z planes for shadows
	float FarZ;
};