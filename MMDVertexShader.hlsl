cbuffer VSData : register(b0)
{
    //float4x4 world;
    float4x4 WV;
    float4x4 WVP;
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
    float3 Pos : TEXCOORD0;
    float3 Nor : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

//change to be in world space
VSOutput main(VSInput input)
{
    VSOutput vsOut;
    vsOut.Position = mul(WVP, float4(input.Pos, 1.0));
    vsOut.Pos = mul(WV, float4(input.Pos, 1.0)).xyz;
    vsOut.Nor = mul((float3x3)WV, input.Nor).xyz;
    vsOut.UV = float2(input.UV.x, 1.0 - input.UV.y);
    //vsOut.UV = input.UV;
    return vsOut;
}
