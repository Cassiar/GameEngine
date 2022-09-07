//only need to pass on position
struct VertexToPixel_Shadow {
	float4 screenPos	: SV_POSITION;
	float4 worldPos		: POSITION;
	float3 lightPos		: LIGHT_POS;
};

struct ShadowOut {
	float4 color : SV_TARGET;
	float depth : SV_DEPTH;
};

ShadowOut main(VertexToPixel_Shadow input)
{
	ShadowOut output;
	output.depth = length(input.lightPos);//length((input.worldPos.xyz - input.lightPos.xyz) / input.worldPos.w);
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	return output;//length(input.lightPos);
}