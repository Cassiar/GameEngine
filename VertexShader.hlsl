#include "Structs.hlsli"

//constant buffer. Can't be changed by shaders but
// can be changed from C++
cbuffer ExternalData : register(b0) {
	matrix world;
	matrix worldInvTranspose;
	matrix view;
	matrix proj;
	matrix lightView;
	matrix lightProj;
	float3 lightPos;
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;

	// 'backwards' due to column major
	matrix wvp = mul(mul(proj, view), world);
	output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

	//position in shadow map
	matrix swvp = mul(mul(lightProj, lightView), world);
	output.shadowPos = mul(swvp, float4(input.localPosition, 1.0f));
	 
	//rotate normal
	//cast to 3x3 to only apply rotatation. Use inverse transpose to ignore non-uniform scale
	output.normal = mul((float3x3)worldInvTranspose, input.normal);
	output.tangent = mul((float3x3)worldInvTranspose, input.tangent);
	//output.bitangent = mul((float3x3)worldInvTranspose, input.bitangent);

	//multiply local pos by world mat and grab first three components
	float4 worldPos = mul(world, float4(input.localPosition, 1));
	output.worldPos = worldPos;

	output.cubePos = worldPos;// mul(wvp, float4(input.localPosition, 1.0f));
	//'reset' to linear depth to make calculating point ligt shadow in ps easier
	output.cubePos.z = length(worldPos.xyz) * worldPos.w / 100.0f;

	//output.cubePos = mul(world, float4(input.localPosition, 1));
	////'reset' to linear depth to make calculating point ligt shadow in ps easier
	//output.cubePos.z = length(output.shadowPos.xyz) * output.screenPosition.w / 100.0f;
	
	output.cubeDepth = length(worldPos - lightPos) / worldPos.w;// abs(length(worldPos.xyz - lightPos) * worldPos.w) / 100;

	// Pass the color through 
	// - The values will be interpolated per-pixel by the rasterizer
	// - We don't need to alter it here, but we do need to send it to the pixel shader
	output.uvCoord = input.uvCoord;

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}