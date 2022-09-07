#include "Structs.hlsli" //for VertexShaderInput struct

// can be changed from C++
//only need view and proj to keep box centered on camera
cbuffer ExternalData : register(b0) {
	matrix view;
	matrix proj;
}

VertexToPixel_Sky main( VertexShaderInput input)
{
	VertexToPixel_Sky output;

	matrix viewNoTranslation = view;
	//zero out translation section
	viewNoTranslation._14 = 0;
	viewNoTranslation._24 = 0;
	viewNoTranslation._34 = 0;

	//apply view and proj transformation. 
	output.position = mul(mul(proj, viewNoTranslation), float4(input.localPosition, 1.0f));
	output.position.z = output.position.w; // insure that the dist vale will be at max
	
	output.sampleDir = input.localPosition; //localPos is just an offset from origin, which is input.localPos
	
	return output;
}