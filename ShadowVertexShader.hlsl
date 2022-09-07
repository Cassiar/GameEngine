#include "Structs.hlsli"

//constant buffer. Can't be changed by shaders but
// can be changed from C++
cbuffer ExternalData : register(b0) {
	matrix world;
	matrix view;
	matrix proj;
	float3 lightPos;
}

//only need to pass on position
struct VertexToPixel_Shadow {
	float4 screenPos	: SV_POSITION;
	float4 worldPos		: POSITION;
	float3 lightPos		: LIGHT_POS;
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel_Shadow main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixel_Shadow output;

	// 'backwards' due to column major
	matrix wvp = mul(mul(proj, view), world);
	output.screenPos = mul(wvp, float4(input.localPosition, 1.0f));

	output.worldPos = mul(world, float4(input.localPosition, 1.0f));

	output.lightPos = lightPos - output.worldPos.xyz;

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}