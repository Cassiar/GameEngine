#pragma once

#include "BufferStructs.h"

#include <d3d11.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//for saba shaders
#include "mmd.vso.h"
#include "mmd.pso.h"
#include "mmd_edge.vso.h"
#include "mmd_edge.pso.h"
#include "mmd_ground_shadow.vso.h"
#include "mmd_ground_shadow.pso.h"

#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <Saba/Model/MMD/PMXModel.h>

struct SabaVertex
{
	glm::vec3	m_position;
	glm::vec3	m_normal;
	glm::vec2	m_uv;
};

struct MMDVertexShaderCB
{
	DirectX::XMFLOAT4X4	m_wv;
	DirectX::XMFLOAT4X4 m_wvp;
};

struct MMDPixelShaderCB
{
	float		m_alpha;
	glm::vec3	m_diffuse;
	glm::vec3	m_ambient;
	float		m_dummy1;
	glm::vec3	m_specular;
	float		m_specularPower;
	glm::vec3	m_lightColor;
	float		m_dummy2;
	glm::vec3	m_lightDir;
	float		m_dummy3;

	glm::vec4	m_texMulFactor;
	glm::vec4	m_texAddFactor;

	glm::vec4	m_toonTexMulFactor;
	glm::vec4	m_toonTexAddFactor;

	glm::vec4	m_sphereTexMulFactor;
	glm::vec4	m_sphereTexAddFactor;

	glm::ivec4	m_textureModes;

};

// mmd edge shader constant buffer

struct MMDEdgeVertexShaderCB
{
	DirectX::XMFLOAT4X4	m_wv;
	DirectX::XMFLOAT4X4	m_wvp;
	glm::vec2	m_screenSize;
	float		m_dummy[2];
};

struct MMDEdgeSizeVertexShaderCB
{
	float		m_edgeSize;
	float		m_dummy[3];
};

struct MMDEdgePixelShaderCB
{
	glm::vec4	m_edgeColor;
};


// mmd ground shadow shader constant buffer

struct MMDGroundShadowVertexShaderCB
{
	DirectX::XMFLOAT4X4	m_wvp;
};

struct MMDGroundShadowPixelShaderCB
{
	glm::vec4	m_shadowColor;
};

struct SabaTexture
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Texture2D>				m_texture;
	ComPtr<ID3D11ShaderResourceView>	m_textureView;
	bool								m_hasAlpha;
};

struct SabaMaterial
{
	explicit SabaMaterial(const saba::MMDMaterial& mat)
		: m_mmdMat(mat)
	{}

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const saba::MMDMaterial& m_mmdMat;
	SabaTexture	m_texture;
	SabaTexture	m_spTexture;
	SabaTexture	m_toonTexture;
};
