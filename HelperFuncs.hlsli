#ifndef HELPER_FUNCS_HLSLI
#define HELPER_FUNCS_HLSLI


//to use Light struct in attenuate function
#include "Structs.hlsli"


// ==============================
// Below are PBR helper functions
// ==============================

// Calculates diffuse amount based on energy conservation
//
// diffuse - Diffuse amount
// specular - Specular color (including light color)
// metalness - surface metalness amount
//
// Metals should have an albedo of (0,0,0)...mostly
// See slide 65: http://blog.selfshadow.com/publications/s2014-shading-course/hoffman/s2014_pbs_physics_math_slides.pdf
float3 DiffuseEnergyConserve(float3 diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

// GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
//
// D(h, n) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float SpecDistribution(float3 n, float3 h, float roughness)
{
	// Pre-calculations
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!
	// ((n dot h)^2 * (a^2 - 1) + 1)
	float denomToSquare = NdotH2 * (a2 - 1) + 1;
	// Can go to zero if roughness is 0 and NdotH is 1; MIN_ROUGHNESS helps here
	// Final value
	return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
//
// v - View vector
// h - Half vector
// f0 - Value when l = n (full specular color)
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 Fresnel(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
	float VdotH = saturate(dot(v, h));
	// Final value
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX (based on Schlick-Beckmann)
// - k is remapped to a / 2, roughness remapped to (r+1)/2
//
// n - Normal
// v - View vector
//
// G(l,v)
float GeometricShadowing(float3 n, float3 v, float roughness)
{
	// End result of remapping:
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(dot(n, v));
	// Final value
	return NdotV / (NdotV * (1 - k) + k);
}

// Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - part of the denominator are canceled out by numerator (see below)
//
// D() - Spec Dist - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 specColor)
{
	// Other vectors
	float3 h = normalize(v + l);
	// Grab various functions
	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor);
	float G = GeometricShadowing(n, v, roughness) * GeometricShadowing(n, l, roughness);
	// Final formula
	// Denominator dot products partially canceled by G()!
	// See page 16: http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf
	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
}

//=========================
//End PBR helper funcs
//=========================


//Basic lambert diffuse shading. Assumings vectors are normalized before being passed in.
float3 Diffuse(float3 normal, float3 dirToLight) {
	return saturate(dot(normal, dirToLight));
}

//cameraPos - where camera is in 3d space
//worldPos - where the pixel is in 3d space
//lightDir - direction light is traveling
//surfaceNormal - normal of the surface
//specExponent - some value to make it shiny
float Phong(float3 cameraPos, float3 worldPos, float3 lightDir, float3 surfaceNormal, float specExponent) {
	//vector from pixel to camera
	float3 V = normalize(cameraPos - worldPos);
	//perfect reflection, first param is direction light is traveling, second is surface normal
	float3 R = reflect(lightDir, surfaceNormal);
	return pow(saturate(dot(R, V)), specExponent);
}

float Attenuate(Light light, float3 worldPos) {
	float dist = distance(light.position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
	return att * att;
}

float SpotFalloff(float3 lightDir, float3 dirToLight, float falloff) {

}

//directional old before pbr
float3 DirectionalOld(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness) {
	float3 lightDir = normalize(light.direction);
	float3 dirToCamera = normalize(cameraPos - worldPos);

	float3 diffuse = Diffuse(normal, -lightDir);
	float3 spec = 0;
	if (specExponent > 0.05) {
		spec = Phong(cameraPos, worldPos, lightDir, normal, specExponent) * roughness;
	}

	return (diffuse * surfaceColor.rgb + spec) * light.intensity * light.color;
}

float3 Directional(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 specColor, float3 surfaceColor, float roughness, float metalness) {
	float3 lightDir = normalize(light.direction);
	float3 dirToCamera = normalize(cameraPos - worldPos);

	float3 diffuse = Diffuse(normal, -lightDir);
	//calculate phong
	float3 spec = 0;
	if (specExponent > 0.05) {
		//phong = Phong(cameraPos, worldPos, lightDir, normal, specExponent) * roughnessMap;
		spec = MicrofacetBRDF(normal, -lightDir, dirToCamera, roughness, specColor);
	}	

	// Calculate diffuse with energy conservation
	// (Reflected light doesn't get diffused)
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, spec, metalness);
	// Combine the final diffuse and specular values for this light
	float3 total = (balancedDiff * surfaceColor.rgb + spec) * light.intensity * light.color;

	return total;
}

float3 DirectionalToon(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness, Texture2D ramp, SamplerState samplerState) {
	float3 lightDir = normalize(light.direction);
	float3 dirToCamera = normalize(cameraPos - worldPos);

	float3 diffuse = Diffuse(normal, -lightDir);

	//use diffuse to calculate position in range band
	float3 rampAmt = ramp.Sample(samplerState, diffuse.xy).rgb;

	float3 spec = 0;
	if (specExponent > 0.05) {
		spec = Phong(cameraPos, worldPos, lightDir, normal, specExponent) * roughness;
	}

	return (rampAmt * surfaceColor.rgb + spec) * light.intensity * light.color;
}

//before pbr
float3 PointOld(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness) {
	float3 dirToLight = normalize(light.position - worldPos);
	float3 dirToCamera = normalize(cameraPos - worldPos);
	float3 diffuse = Diffuse(normal, dirToLight);
	float3 spec = 0;
	if (specExponent > 0.05) {
		spec = Phong(cameraPos, worldPos, -dirToLight, normal, specExponent) * roughness;
	}

	float3 total = (diffuse * surfaceColor.rgb + spec) * light.intensity * light.color;

	return total * Attenuate(light, worldPos);
}

float3 PointToon(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness, Texture2D ramp, SamplerState samplerState) {
	float3 dirToLight = normalize(light.position - worldPos);
	float3 dirToCamera = normalize(cameraPos - worldPos);
	float3 diffuse = Diffuse(normal, dirToLight);

	float3 rampAmt = ramp.Sample(samplerState, diffuse.xy);
	float3 spec = 0;
	if (specExponent > 0.05) {
		spec = Phong(cameraPos, worldPos, -dirToLight, normal, specExponent) * roughness;
	}

	float3 total = (rampAmt * surfaceColor.rgb + spec) * light.intensity * light.color;

	return total * Attenuate(light, worldPos);
}

float3 Point(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 specColor, float3 surfaceColor, float roughness, float metalness) {
	float3 dirToLight = normalize(light.position - worldPos);
	float3 dirToCamera = normalize(cameraPos - worldPos);
	float3 diffuse = Diffuse(normal, dirToLight);
	float3 spec = 0;
	if (specExponent > 0.05) {
		//spec = Phong(cameraPos, worldPos, -dirToLight, normal, specExponent) * roughnessMap;
		spec = MicrofacetBRDF(normal, dirToLight, dirToCamera, roughness, specColor);
	}

	// Calculate diffuse with energy conservation
// (Reflected light doesn't get diffused)
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, spec, metalness);
	// Combine the final diffuse and specular values for this light
	float3 total = (balancedDiff * surfaceColor.rgb + spec) * light.intensity * light.color;

	return total * Attenuate(light, worldPos);
}

//before pbr
float3 SpotOld(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness) {
	float3 dirToLight = normalize(light.position - worldPos);
	//compare light dir to pixel to true direction 
	float angle = max(dot(-dirToLight, light.direction), 0.0f);

	//raise by power to get a nice falloff
	float spotAmount = pow(angle, light.spotFalloff);

	return PointOld(light, normal, cameraPos, worldPos, specExponent, surfaceColor, roughness) * spotAmount;
}

float3 SpotToon(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness, Texture2D ramp, SamplerState samplerState) {
	float3 dirToLight = normalize(light.position - worldPos);
	//compare light dir to pixel to true direction 
	float angle = max(dot(-dirToLight, light.direction), 0.0f);

	//raise by power to get a nice falloff
	float spotAmount = pow(angle, light.spotFalloff);
	//PointToon(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 surfaceColor, float roughness, Texture2D ramp, SamplerState samplerState)
	return PointToon(light, normal, cameraPos, worldPos, specExponent, surfaceColor, roughness, ramp, samplerState) * spotAmount;
}

float3 Spot(Light light, float3 normal, float3 cameraPos, float3 worldPos, float specExponent, float3 specColor, float3 surfaceColor, float roughness, float metalness) {
	float3 dirToLight = normalize(light.position - worldPos);
	//compare light dir to pixel to true direction 
	float angle = max(dot(-dirToLight, light.direction), 0.0f);

	//raise by power to get a nice falloff
	float spotAmount = pow(angle, light.spotFalloff);

	return Point(light, normal, cameraPos, worldPos, specExponent, specColor, surfaceColor, roughness, metalness) * spotAmount;
}


#endif