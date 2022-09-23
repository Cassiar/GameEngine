#include "Structs.hlsli"
#include "HelperFuncs.hlsli"

cbuffer PSColorBuf : register(b0) {
	float4 colorTint;
	float3 cameraPos;
	float roughness;
	float3 ambientTerm;
	int numLights;
	Light lights[MAX_LIGHTS_NUM];
}

//texture external data
Texture2D AlbedoTexture : register(t0);
Texture2D RoughnessTexture : register(t1);
//Texture2D AmbientTexture : register(t2);
Texture2D NormalTexture : register(t3);
//Texture2D MetalnessTexture : register(t4);
Texture2D ShadowMap : register(t5);

SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	//renormailze normals because they get changed in rasterizer stage.
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	//input.bitangent = normalize(input.bitangent);

	float3 N = input.normal;
	float3 T = normalize(input.tangent - dot(input.tangent, N) * N); //make it ortho-normal to normal
	float3 B = cross(T, N);

	float3 albedoColor = AlbedoTexture.Sample(BasicSampler, input.uvCoord).rgb;

	//calculate surface color
	float3 surfaceColor = pow(albedoColor,2.2f) * colorTint.rgb;

	float roughnessMap = RoughnessTexture.Sample(BasicSampler, input.uvCoord).x;
	//float metalnessMap = MetalnessTexture.Sample(BasicSampler, input.uvCoord).x;

	//unpack normals from texture sample
	float3 unpackedNormal = NormalTexture.Sample(BasicSampler, input.uvCoord).r;// *2.0f - 1.0f;

	//float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal); //tan and bitan calculated when model loaded
	float3x3 TBN = float3x3(T, B, N);

	//final normal so it faes same direction as obj
	//input.normal = mul(unpackedNormal, TBN);
	// calculate specular exponent
	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	// Specular color determination -----------------
// Assume albedo texture is actually holding specular color where metalness == 1
//
// Note the use of lerp here - metal is generally 0 or 1, but might be in between
// because of linear texture sampling, so we lerp the specular color to match
	//float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalnessMap);

	float3 final = ambientTerm;// *AmbientTexture.Sample(BasicSampler, input.uvCoord).rgb;

	//calculate shadow stuff
	//convert form [-1,1] to [0,1]
	float2 shadowMapUV = input.shadowPos.xy / input.shadowPos.w * 0.5f + 0.5f;
	//flip y since we're in uv space and y is flipped
	shadowMapUV.y = 1.0f - shadowMapUV.y;

	//test how far from ligt
	float depthFromLight = input.shadowPos.z / input.shadowPos.w;
	//use comparison sampler to check if pixel is in shadow
	float shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowMapUV, depthFromLight);

	for (int i = 0; i < numLights || i < MAX_LIGHTS_NUM; i++) {
		float3 lightAmount = 0;
		if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
			lightAmount += DirectionalOld(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, surfaceColor, roughnessMap);
		}
		else if (lights[i].type == LIGHT_TYPE_POINT) {
			lightAmount += PointOld(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, surfaceColor, roughnessMap);
		}
		else if (lights[i].type == LIGHT_TYPE_SPOT) {
			lightAmount += SpotOld(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, surfaceColor, roughnessMap);
		}
		if (lights[i].castsShadows) {
			//lightAmount *= shadowAmount;
		}
		final += lightAmount;
	}

	return float4(pow(final, 1.0f / 2.2f), 1.0);
}