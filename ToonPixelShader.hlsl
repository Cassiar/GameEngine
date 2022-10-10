// @author Cassiar Beaver
// a pixel shader for toon shading

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
Texture2D AmbientTexture : register(t2);
//Texture2D NormalTexture : register(t3);
Texture2D RampTexture : register(t3);
Texture2D MetalnessTexture : register(t4);
Texture2D ShadowMap : register(t5);
Texture2D ShadowSpotMap : register(t6);
//TextureCubeArray ShadowBoxes : register(t7);
//TextureCube ShadowBox[MAX_POINT_SHADOWS_NUM] : register(t7);
TextureCube ShadowBox1 : register(t7);
TextureCube ShadowBox2 : register(t8);

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
	float metalnessMap = MetalnessTexture.Sample(BasicSampler, input.uvCoord).x;

	//unpack normals from texture sample
	//float3 unpackedNormal = NormalTexture.Sample(BasicSampler, input.uvCoord).rgb * 2.0f - 1.0f;

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
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalnessMap);

	float3 final = ambientTerm * AmbientTexture.Sample(BasicSampler, input.uvCoord).rgb;

	float numPointShadows = 0;

	for (int i = 0; i < numLights && i < MAX_LIGHTS_NUM; i++) {
		float3 lightAmount = 0;
		if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
			lightAmount += DirectionalToon(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, surfaceColor, roughnessMap, RampTexture, BasicSampler);
			/*
			float3 lightDir = normalize(lights[i].direction);
			float3 dirToCamera = normalize(cameraPos - input.worldPos.xyz);

			float3 diffuse = Diffuse(input.normal, -lightDir);

			//use diffuse to calculate position in range band
			float3 rampAmt = RampTexture.Sample(BasicSampler, diffuse.xy).rgb;

			float3 spec = 0;
			if (specExponent > 0.05) {
				spec = Phong(cameraPos, input.worldPos.xyz, lightDir, input.normal, specExponent) * roughnessMap;
			}

			lightAmount += (rampAmt * surfaceColor.rgb + spec) * lights[i].intensity * lights[i].color;
			*/
			//lightAmount += Directional(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, specColor, surfaceColor, roughnessMap, metalnessMap);
		}
		else if (lights[i].type == LIGHT_TYPE_POINT) {
			lightAmount += PointToon(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, surfaceColor, roughnessMap, RampTexture, BasicSampler);
			//lightAmount += Point(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, specColor, surfaceColor, roughnessMap, metalnessMap);
		}
		else if (lights[i].type == LIGHT_TYPE_SPOT) {
			lightAmount += SpotToon(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, surfaceColor, roughnessMap, RampTexture, BasicSampler);
			//lightAmount += Spot(lights[i], input.normal, cameraPos, input.worldPos.xyz, specExponent, specColor, surfaceColor, roughnessMap, metalnessMap);
		}

		if (lights[i].castsShadows) {
			float shadowAmount = 1.0f; //value of one doesn't change light, value of 0 means no light
			if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
				// calculate shadow stuff
				//convert form [-1,1] to [0,1]
				float2 shadowMapUV = input.shadowPos.xy / input.shadowPos.w * 0.5f + 0.5f;
				//flip y since we're in uv space and y is flipped
				shadowMapUV.y = 1.0f - shadowMapUV.y;

				//test how far from ligt
				float depthFromLight = input.shadowPos.z / input.shadowPos.w;
				//use comparison sampler to check if pixel is in shadow
				shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowMapUV, depthFromLight);
			}
			//make sure we don't try to render more than max amount of point shadows
			else if (lights[i].type == LIGHT_TYPE_POINT) {
				float3 dirToLight = lights[i].position - input.worldPos.xyz;

				//may all the gods bless this stackoverflow post https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap

				//recalculate position in spac when shadow map was rendered
				float3 absDirToLight = abs(dirToLight);
				float localZcomp = max(absDirToLight.x, max(absDirToLight.y, absDirToLight.z));
				float near = lights[i].nearZ;
				float far = lights[i].farZ;

				float NormZComp = (far + near) / (far - near) - (2 * far * near) / (far - near) / localZcomp;
				float distance = (NormZComp + 1.0) * 0.5;

				float4 lightDirection = 0.0f;
				float3 worldPos = input.worldPos.xyz;
				worldPos.z = input.worldPos.z / input.worldPos.w;

				//int index = lights[i].shadowNumber;
				lightDirection.xyz = input.worldPos.xyz - float3(lights[i].position.xy, lights[i].position.z);
				if (numPointShadows == 0) {
					shadowAmount = ShadowBox1.SampleCmpLevelZero(ShadowSampler, -normalize(dirToLight), distance);
				}
				else if (numPointShadows == 1) {
					shadowAmount = ShadowBox2.SampleCmpLevelZero(ShadowSampler, -normalize(dirToLight), distance);
				}
				//shadowAmount = ShadowBoxes[index].SampleCmpLevelZero(ShadowSampler, -normalize(dirToLight), distance);
				//if(lights[i].shadowNumber >= 0 && lights[i].shadowNumber < MAX_POINT_SHADOWS_NUM) {
				//	shadowAmount = ShadowBox[numPointShadows].SampleCmpLevelZero(ShadowSampler, -normalize(dirToLight), distance);
				//}
				numPointShadows++; //count up number of rendered point shadows
			}
			else if (lights[i].type == LIGHT_TYPE_SPOT) {
				// calculate shadow stuff
				//convert form [-1,1] to [0,1]
				float2 shadowMapUV = input.spotShadowPos.xy / input.spotShadowPos.w * 0.5f + 0.5f;
				//flip y since we're in uv space and y is flipped
				shadowMapUV.y = 1.0f - shadowMapUV.y;

				float3 dirToLight = lights[i].position - input.worldPos.xyz;

				//may all the gods bless this stackoverflow post https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap

				float3 absDirToLight = abs(dirToLight);
				float localZcomp = max(absDirToLight.x, max(absDirToLight.y, absDirToLight.z));
				float near = lights[i].nearZ;
				float far = lights[i].farZ;

				float NormZComp = (far + near) / (far - near) - (2 * far * near) / (far - near) / localZcomp;
				float distance = (NormZComp + 1.0) * 0.5;

				//test how far from ligt
				float depthFromLight = input.spotShadowPos.z / input.spotShadowPos.w;
				//use comparison sampler to check if pixel is in shadow
				shadowAmount = ShadowSpotMap.SampleCmpLevelZero(ShadowSampler, shadowMapUV, depthFromLight);
			}

			lightAmount *= shadowAmount;
		}
		final += lightAmount;
	}


	return float4(pow(final, 1.0f / 2.2f), 1.0);
}