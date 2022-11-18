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

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Pos : TEXCOORD0;
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
    float3 eyeDir = normalize(vsOut.Pos);
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
            float3 dirToLight = normalize(lights[i].position - vsOut.Pos);
            lightDir = -dirToLight;
        }
        else if (lights[i].type == LIGHT_TYPE_SPOT) {
            float3 dirToLight = normalize(lights[i].position - vsOut.Pos);
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
        final += colorWithLight;
    }

    return float4(final, alpha);
}
