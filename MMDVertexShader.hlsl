cbuffer VSData : register(b0)
{
    float4x4 W;
    float4x4 WV;
    float4x4 WVP;
    matrix lightView;//directional shadow
    matrix lightProj;
    matrix spotLightView; //spot shadow
    matrix spotLightProj;
};

struct VSInput
{
    float3 Pos : POSITION;
    float3 Nor : NORMAL; // normal vector
    float3 tangent			: TANGENT;
    float2 UV : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 shadowPos		: SHADOW_POSITION;
    float4 spotShadowPos	: SPOT_SHADOW_POSITION;
    float4 Pos : TEXCOORD0;
    float3 Nor : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

//change to be in world space
VSOutput main(VSInput input)
{
    VSOutput vsOut;
    vsOut.Position = mul(WVP, float4(input.Pos, 1.0));
    vsOut.Pos = mul(W, float4(input.Pos, 1.0));
    vsOut.Nor = mul((float3x3)W, input.Nor).xyz;
    vsOut.UV = float2(input.UV.x, 1.0 - input.UV.y);

    matrix swvp = mul(mul(lightProj, lightView), W);
    vsOut.shadowPos = mul(swvp, float4(input.Pos, 1.0f));

    matrix spotswvp = mul(mul(spotLightProj, spotLightView), W);
    vsOut.spotShadowPos = mul(spotswvp, float4(input.Pos, 1.0f));

    //vsOut.UV = input.UV;
    return vsOut;
}
