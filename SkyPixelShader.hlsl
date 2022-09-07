#include "Structs.hlsli"

//cube map resource
TextureCube SkyBox : register(t0);
//sampler
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel_Sky input) : SV_TARGET
{
	return SkyBox.Sample(BasicSampler, input.sampleDir);
}