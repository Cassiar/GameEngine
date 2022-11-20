#include "Structs.hlsli"

cbuffer PSData : register(b1)
{
    float3  Diffuse;
    float   Alpha;
    float3  Ambient;
    int numLights;
    float3  Specular;
    float   SpecularPower;
    //float3  LightColor;
    //float3  LightDir;

    float4  TexMulFactor;
    float4  TexAddFactor;

    float4  ToonTexMulFactor;
    float4  ToonTexAddFactor;

    float4  SphereTexMulFactor;
    float4  SphereTexAddFactor;

    int4    TextureModes;
    Light lights[MAX_LIGHTS_NUM];
}

Texture2D Tex : register(t0);
Texture2D ToonTex : register(t1);
Texture2D SphereTex : register(t2);
sampler TexSampler : register(s0);
sampler ToonTexSampler : register(s1);
sampler SphereTexSampler : register(s2);

//shadows!
Texture2D ShadowMap : register(t5);
Texture2D ShadowSpotMap : register(t6);
TextureCube ShadowBox1 : register(t7);
TextureCube ShadowBox2 : register(t8);

SamplerComparisonState ShadowSampler : register(s3);

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 shadowPos		: SHADOW_POSITION;
    float4 spotShadowPos	: SPOT_SHADOW_POSITION;
    float4 Pos : TEXCOORD0;
    float3 Nor : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

float3 ComputeTexMulFactor(float3 texColor, float4 factor)
{
    float3 ret = texColor * factor.rgb;
    return lerp(float3(1.0, 1.0, 1.0), ret, factor.a);
}

float3 ComputeTexAddFactor(float3 texColor, float4 factor)
{
    float3 ret = texColor + (texColor - (float3)1.0) * factor.a;
    ret = clamp(ret, (float3)0.0, (float3)1.0) + factor.rgb;
    return ret;
}

float4 main(VSOutput vsOut) : SV_TARGET0
{
    float3 eyeDir = normalize(vsOut.Pos.xyz);
    float3 nor = normalize(vsOut.Nor);
    float3 color = float3(0.0, 0.0, 0.0);
    float alpha = Alpha;
    float3 diffuseColor = 0;
    color += Ambient;
    int TexMode = TextureModes.x;
    int ToonTexMode = TextureModes.y;
    int SphereTexMode = TextureModes.z;

    if (TexMode != 0)
    {
        float4 texColor = Tex.Sample(TexSampler, vsOut.UV);
        texColor.rgb = ComputeTexMulFactor(texColor.rgb, TexMulFactor);
        texColor.rgb = ComputeTexAddFactor(texColor.rgb, TexAddFactor);
        color = texColor.rgb;
        if (TexMode == 2)
        {
            alpha *= texColor.a;
        }
    }
    //return float4(1.0f,0,0,0);
    if (alpha == 0.0)
    {
        discard;
    }

    if (SphereTexMode != 0)
    {
        float2 spUV = (float2)0.0;
        spUV.x = nor.x * 0.5 + 0.5;
        spUV.y = nor.y * 0.5 + 0.5;
        float3 spColor = SphereTex.Sample(SphereTexSampler, spUV).rgb;
        spColor = ComputeTexMulFactor(spColor, SphereTexMulFactor);
        spColor = ComputeTexAddFactor(spColor, SphereTexAddFactor);
        if (SphereTexMode == 1)
        {
            color *= spColor;
        }
        else if (SphereTexMode == 2)
        {
            color += spColor;
        }
    }

    //return float4(color,alpha);

    float3 final = float3(0, 0, 0);

    int numPointShadows = 0;

    for (int i = 0; i < numLights && i < MAX_LIGHTS_NUM; i++) {
        float3 colorWithLight = color;
        float3 lightDir = 0;
        float ln = 0;
        //set to 1 to say this isn't a spot light
        //we'll multiple the light by this to get actuall spot amount
        float spotAmount = 1; 
        if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
            lightDir = normalize(-lights[i].direction);

        }
        else if (lights[i].type == LIGHT_TYPE_POINT) {
            float3 dirToLight = normalize(lights[i].position - vsOut.Pos.xyz);
            lightDir = -dirToLight;
        }
        else if (lights[i].type == LIGHT_TYPE_SPOT) {
            float3 dirToLight = normalize(lights[i].position - vsOut.Pos.xyz);
            //compare light dir to pixel to true direction 
            float angle = max(dot(-dirToLight, lights[i].direction), 0.0f);
            lightDir = -dirToLight;
            //raise by power to get a nice falloff
            spotAmount = pow(angle, lights[i].spotFalloff);
        }
        
        ln = dot(nor, lightDir);
        //ln = clamp(ln + 0.5, 0.0, 1.0);
        ln = ln * 0.5f + 0.5f;
        //diffuseColor = Diffuse * lights[i].color;
        //color += lights[i].color * lights[i].intensity;


        if (ToonTexMode != 0)
        {
            float3 toonColor = ToonTex.Sample(ToonTexSampler, float2(0.0, 1.0 - ln)).rgb;
            toonColor = ComputeTexMulFactor(toonColor, ToonTexMulFactor);
            toonColor = ComputeTexAddFactor(toonColor, ToonTexAddFactor);
            colorWithLight *= toonColor;
        }

        float3 specular = (float3)0.0;
        if (SpecularPower > 0.0)
        {
            float3 halfVec = normalize(eyeDir + lightDir);
            float3 specularColor = Specular * lights[i].color;// LightColor;
            specular += pow(max(0.0, dot(halfVec, nor)), SpecularPower) * specularColor;
        }
        colorWithLight += specular;

        colorWithLight *= lights[i].color * lights[i].intensity * spotAmount;

        if (lights[i].castsShadows) {
            float shadowAmount = 1.0f; //value of one doesn't change light, value of 0 means no light
            if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
                // calculate shadow stuff
                //convert form [-1,1] to [0,1]
                float2 shadowMapUV = vsOut.shadowPos.xy / vsOut.shadowPos.w * 0.5f + 0.5f;
                //flip y since we're in uv space and y is flipped
                shadowMapUV.y = 1.0f - shadowMapUV.y;

                //test how far from ligt
                float depthFromLight = vsOut.shadowPos.z / vsOut.shadowPos.w;
                //use comparison sampler to check if pixel is in shadow
                shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowMapUV, depthFromLight);
            }
            //make sure we don't try to render more than max amount of point shadows
            else if (lights[i].type == LIGHT_TYPE_POINT) {
                float3 dirToLight = lights[i].position - vsOut.Pos.xyz;

                //may all the gods bless this stackoverflow post https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap

                //recalculate position in spac when shadow map was rendered
                float3 absDirToLight = abs(dirToLight);
                float localZcomp = max(absDirToLight.x, max(absDirToLight.y, absDirToLight.z));
                float near = lights[i].nearZ;
                float far = lights[i].farZ;

                float NormZComp = (far + near) / (far - near) - (2 * far * near) / (far - near) / localZcomp;
                float distance = (NormZComp + 1.0) * 0.5;

                float4 lightDirection = 0.0f;
                float3 worldPos = vsOut.Pos.xyz;
                worldPos.z = vsOut.Pos.z / vsOut.Pos.w;

                //int index = lights[i].shadowNumber;
                lightDirection.xyz = vsOut.Pos.xyz - float3(lights[i].position.xy, lights[i].position.z);
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
                float2 shadowMapUV = vsOut.spotShadowPos.xy / vsOut.spotShadowPos.w * 0.5f + 0.5f;
                //flip y since we're in uv space and y is flipped
                shadowMapUV.y = 1.0f - shadowMapUV.y;

                float3 dirToLight = lights[i].position - vsOut.Pos.xyz;

                //may all the gods bless this stackoverflow post https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap

                float3 absDirToLight = abs(dirToLight);
                float localZcomp = max(absDirToLight.x, max(absDirToLight.y, absDirToLight.z));
                float near = lights[i].nearZ;
                float far = lights[i].farZ;

                float NormZComp = (far + near) / (far - near) - (2 * far * near) / (far - near) / localZcomp;
                float distance = (NormZComp + 1.0) * 0.5;

                //test how far from ligt
                float depthFromLight = vsOut.spotShadowPos.z / vsOut.spotShadowPos.w;
                //use comparison sampler to check if pixel is in shadow
                shadowAmount = ShadowSpotMap.SampleCmpLevelZero(ShadowSampler, shadowMapUV, depthFromLight);
            }

            colorWithLight *= shadowAmount;
        }
        final += colorWithLight;
    }

    return float4(final, alpha);
}
