//@source https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-13-volumetric-light-scattering-post-process
//@purpose post process shader to handle drawing light rays
//made for IGME 550 game engine design and development

#include "Structs.hlsli"

#define NUM_SAMPLES 200
//const float Density = 1.0;
//const float Weight = 1.0;

cbuffer PPLightRaysBuf : register(b0) {
	float density;
	float weight;
	float decay;
	float exposure;
}

Texture2D ScreenTexture : register(t0); //screen image

SamplerState BasicSampler : register(s0); //basic sampler

float4 main(VertexToPixel_PPLightRays input) : SV_TARGET
{   
	// Calculate vector from pixel to light source in screen space.    
	half2 deltaTexCoord = (input.texCoord - input.lightScreenPos);   

	// Divide by number of samples and scale by control factor.   
	deltaTexCoord *= 1.0f / NUM_SAMPLES * density;   
	
	// Store initial sample.    
	float3 color = ScreenTexture.Sample(BasicSampler, input.texCoord);
	//return float4(color.rgb,1);
	
	// Set up illumination decay factor.    
	half illuminationDecay = 1.0f;   
	
	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.    
	
	for (int i = 0; i < NUM_SAMPLES; i++)   {     
		// Step sample location along ray.     
		input.texCoord -= deltaTexCoord;     
		
		// Retrieve sample at new location.    
		float3 newColor = ScreenTexture.Sample(BasicSampler, input.texCoord);
		
		// Apply sample attenuation scale/decay factors.     
		newColor *= illuminationDecay * weight;
		
		// Accumulate combined color.     
		color += newColor;
		
		// Update exponential decay factor.     
		illuminationDecay *= decay;   
	}   
	
	// Output final color with a further scale control factor.    
	return float4( color * exposure, 1);
} 
