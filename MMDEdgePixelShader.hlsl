cbuffer PSData : register(b2)
{
    float4 EdgeColor;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

float4 main(VSOutput vsOut) : SV_TARGET0
{
    return EdgeColor;
}
