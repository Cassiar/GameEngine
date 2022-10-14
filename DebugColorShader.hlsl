#include "Structs.hlsli"

cbuffer ColorBuf : register(b0) {
	float4 colorTint;
}
float4 main(VertexToPixel input) : SV_TARGET
{
	return colorTint;
}