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
Texture2D NormalTexture : register(t3);
Texture2D MetalnessTexture : register(t4);
Texture2D ShadowMap : register(t5);
TextureCube ShadowBox : register(t6);

//changeling stuff
Texture2D TFlow : register(t7);
Texture2D Pigment : register(t8);
Texture1D Ramp : register(t9);

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
	float3 unpackedNormal = NormalTexture.Sample(BasicSampler, input.uvCoord).rgb * 2.0f - 1.0f;

	//float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal); //tan and bitan calculated when model loaded
	float3x3 TBN = float3x3(T, B, N);

	//final normal so it faes same direction as obj
	input.normal = mul(unpackedNormal, TBN);
	// calculate specular exponent
	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	// Specular color determination -----------------
// Assume albedo texture is actually holding specular color where metalness == 1
//
// Note the use of lerp here - metal is generally 0 or 1, but might be in between
// because of linear texture sampling, so we lerp the specular color to match
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalnessMap);

	float3 final = ambientTerm * AmbientTexture.Sample(BasicSampler, input.uvCoord).rgb;

	for (int i = 0; i < numLights && i < MAX_LIGHTS_NUM; i++) {
		float3 lightAmount = 0;
		if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
			float3 lightDir = normalize(lights[i].direction);
			float3 dirToCamera = normalize(cameraPos - input.worldPos);

			float diffuse = Diffuse(input.normal, -lightDir);
			diffuse = Ramp.Sample(BasicSampler, diffuse);
			float3 spec = 0;
			if (specExponent > 0.05) {
				spec = Phong(cameraPos, input.worldPos, lightDir, input.normal, specExponent) * roughness;
			}

			lightAmount += (diffuse * surfaceColor.rgb + spec) * lights[i].intensity * lights[i].color;
			//lightAmount += Directional(lights[i], input.normal, cameraPos, input.worldPos, specExponent, specColor, surfaceColor, roughnessMap, metalnessMap);
		}
		/*else if (lights[i].type == LIGHT_TYPE_POINT) {
			lightAmount += Point(lights[i], input.normal, cameraPos, input.worldPos, specExponent, specColor, surfaceColor, roughnessMap, metalnessMap);
		}
		else if (lights[i].type == LIGHT_TYPE_SPOT) {
			lightAmount += Spot(lights[i], input.normal, cameraPos, input.worldPos, specExponent, specColor, surfaceColor, roughnessMap, metalnessMap);
		}*/
		if (lights[i].castsShadows) {
			float shadowAmount = 1.0f;
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
			else if (lights[i].type == LIGHT_TYPE_POINT) {
				float3 dirToLight = lights[i].position - input.worldPos;

				//may all the gods bless this stackoverflow post https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap

				float3 absDirToLight = abs(dirToLight);
				float localZcomp = max(absDirToLight.x, max(absDirToLight.y, absDirToLight.z));
				float near = 0.5f;
				float far = 100.0f;

				float NormZComp = (far + near) / (far - near) - (2 * far * near) / (far - near) / localZcomp;
				float distance = (NormZComp + 1.0) * 0.5;

				float4 lightDirection = 0.0f;
				float3 worldPos = input.worldPos.xyz;// / input.worldPos.w;
				worldPos.z = input.worldPos.z / input.worldPos.w;


				lightDirection.xyz = input.worldPos.xyz - float3(lights[i].position.xy, lights[i].position.z);
				//depth
				//distance = length(lightDirection.xyz);// / input.worldPos.w;// / input.worldPos.w;//length(lights[i].position.xyz - worldPos.xyz);// / input.worldPos.w;
				//attenuation factor
				//lightDirection.xyz = lightDirection.xyz / distance;
				//lightDirection.w = max(0,1/(lightDirection.x + att))

				//float depthFromLight = abs(length((input.worldPos.xyz / input.worldPos.w) - lights[i].position));// / input.worldPos.w;// / input.cubePos.w;//length(input.cubePos.xyz)/input.cubePos.w
				shadowAmount = ShadowBox.SampleCmpLevelZero(ShadowSampler, -normalize(dirToLight), distance);
				//shadowAmount = ShadowBox.Sample(BasicSampler, -normalize(dirToLight));// *input.worldPos.w;

				/*return shadowAmount.rrrr;
				if (shadowAmount + 0.0001f < distance) {
					//we're in shadow
					return float4(0, 0, 0, 0);
				}
				else {
					return float4(1, 1, 1, 1);
				}*/
			}
			lightAmount *= shadowAmount;
		}
		final += lightAmount;
	}


	//calculate the watercolor effect for changeling project

	//calculate pigment density
	float d = 1 + 0.25 * (TFlow.Sample(BasicSampler, input.uvCoord).r - 0.5f);
	float c = 1 + 1 * (Pigment.Sample(BasicSampler, input.uvCoord).r - 0.5f);

	//return c.rrrr;
	//calculate new color C-(C-C^2)(d-1)
	float3 wFinal = final - (final - pow(final, 2)) * (d - 1);
	//wFinal += final - (final - pow(final, 2)) * (c - 1);

	return float4(pow(final, 1.0f / 2.2f), 1.0);
}