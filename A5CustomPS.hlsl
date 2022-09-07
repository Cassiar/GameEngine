
cbuffer PSColorBuf : register(b0) {
	float4 colorTint;
}

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;
	float2 uvCoord			: TEXCOORD;
};

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
	//create custom 'art' effect

	//remap uvs to be in one circle range [0, 2*pi)
	float u = input.uvCoord.x * 2 * 3.14f;
	float v = input.uvCoord.y * 2 * 3.14f;

	//float red = input.uvCoord.x * input.uvCoord.y + 0.1; // circle gradient centered at bottom left corner
	float blue = (input.uvCoord.x) / (input.uvCoord.y + 1); //similar to above but at bottom right
	float green = -16 * pow(input.uvCoord.y - 0.5, 2) + 1; //make stripe in middle vertically

	float red = (sin(u) * sin(v));
	blue = cos(u) * sin(v);
	green = sin(u + 3.14f / 2) * cos(v + 3.14f / 2); //shift right and up by quarter width



	// Just returning the uv coord for testing
	return float4(red, green, blue, 1);
}