#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI


// The fresnel value for non-metals (dielectrics)
// Page 9: "F0 of nonmetals is now a constant 0.04"
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

// Handy to have this as a constant
static const float PI = 3.14159265359f;


#define MAX_LIGHTS_NUM 10
#define MAX_POINT_SHADOWS_NUM 2

//struct shared between vertex and pixel shader
//which passes info from vertex to pixel shader.
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;
	float4 shadowPos		: SHADOW_POSITION;
	float4 spotShadowPos	: SPOT_SHADOW_POSITION;
	float4 worldPos			: POSITION;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	//float3 bitangent		: BITANGENT;
	float2 uvCoord			: TEXCOORD;
};

//major light types
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

//max specular exponent value
#define MAX_SPECULAR_EXPONENT 256.0f;

//define struct to hold needed data for our lights
struct Light {
	int type				: TYPE; //which kind of light, use those above
	float3 direction		: DIRECTION; //direction and spot lights need to know where they're pointing
	float range				: RANGE; //spot and point have max range
	float3 position			: POSITION; //spot and point need to kow location in 3d space.
	float intensity			: INTENSITY; //All need this
	float3 color			: COLOR; //all lights need a color
	float spotFalloff		: SPOTFALLOFF; //spot lights need to have cone size
	float nearZ				: NEARZ; //near and far z values for shadow maps
	float farZ				: FARZ;
	//int shadowNumber		: SHADOWNUMBER; //which point shadow map to use
	bool castsShadows		: CASTSHADOW;
};

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;     // XYZ position
	float3 normal			: NORMAL;        // normal vector
	float3 tangent			: TANGENT;
	//float3 bitangent		: BITANGENT;
	float2 uvCoord			: TEXCOORD;		//uv coordinate
};

//special struct for our sky shaders
struct VertexToPixel_Sky {
	float4 position : SV_POSITION; // position of pixel
	float3 sampleDir : DIRECTION; //direction to sample sky box
};

#endif