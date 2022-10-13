//@author Cassiar Beaver cdb7951
//@purpose convert a triangle view of the screen into usuable coordinates

#include "Structs.hlsli"

//constant buffer. Can't be changed by shaders but
// can be changed from C++
//world proj and view are from light's perspective
cbuffer ExternalData : register(b0) {
	matrix world;
	//matrix worldInvTranspose;
	matrix proj;
	matrix view;
	float3 lightPos;
}

VertexToPixel_PPLightRays main(uint id : SV_VERTEXID)
{
	VertexToPixel_PPLightRays output;

	//convert light world pos to screen pos
	matrix wvp = mul(mul(proj, view), world);
	//output.lightScreenPos = mul(wvp, float4(lightPos, 1.0f));
	//output.lightScreenPos = mul(wvp, float4(1.0f, 1.0f, 1.0f, 1.0f));
	
	float4 tempPos = mul(mul(proj, view), float4(lightPos, 1.0f));
	output.lightScreenPos = tempPos.xy / tempPos.w;

	output.lightScreenPos = output.lightScreenPos * 0.5f + 0.5f;
	output.lightScreenPos.y = 1 - output.lightScreenPos.y;
	//output.lightScreenPos = float2(0.5f, 0.5f);

	//============================
	// Below code Author: Chris Cascioli
	//============================
	
	// Calculate the UV (0,0 to 2,2) via the ID
	// x = 0, 2, 0, 2, etc.
	// y = 0, 0, 2, 2, etc.
	output.texCoord = float2((id << 1) & 2, id & 2);

	// Convert uv to the (-1,1 to 3,-3) range for position
	output.position = float4(output.texCoord, 0, 1);
	output.position.x = output.position.x * 2 - 1;
	output.position.y = output.position.y * -2 + 1;

	//output.shadowPos = mul(wvp, output.position);

	return output;
}