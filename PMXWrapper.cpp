#include "PMXWrapper.h"

PMXWrapper::PMXWrapper(const char* path, const char* texpath)
{
	model->Load(path, texpath);
}

void PMXWrapper::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	//const auto& view = appContext.m_viewMat;
	//const auto& proj = appContext.m_projMat;
	//const auto& dxMat = glm::mat4(
	//	1.0f, 0.0f, 0.0f, 0.0f,
	//	0.0f, 1.0f, 0.0f, 0.0f,
	//	0.0f, 0.0f, 0.5f, 0.0f,
	//	0.0f, 0.0f, 0.5f, 1.0f
	//);

	//auto world = glm::mat4(1.0f);
	//auto wv = view * world;
	//auto wvp = dxMat * proj * view * world;
	//auto wvit = glm::mat3(view * world);
	//wvit = glm::inverse(wvit);
	//wvit = glm::transpose(wvit);

	//// Set viewport
	//D3D11_VIEWPORT vp;
	//vp.Width = float(appContext.m_screenWidth);
	//vp.Height = float(appContext.m_screenHeight);
	//vp.MinDepth = 0.0f;
	//vp.MaxDepth = 1.0f;
	//vp.TopLeftX = 0;
	//vp.TopLeftY = 0;
	//context->RSSetViewports(1, &vp);
	//ID3D11RenderTargetView* rtvs[] = { appContext.m_renderTargetView.Get() };
	//context->OMSetRenderTargets(1, rtvs,
	//	appContext.m_depthStencilView.Get()
	//);

	//context->OMSetDepthStencilState(appContext.m_defaultDSS.Get(), 0x00);

	//// Setup input assembler
	//{
	//	UINT strides[] = { sizeof(saba::Vertex) };
	//	UINT offsets[] = { 0 };
	//	context->IASetInputLayout(appContext.m_mmdInputLayout.Get());
	//	ID3D11Buffer* vbs[] = { m_vertexBuffer.Get() };
	//	context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	//	context->IASetIndexBuffer(m_indexBuffer.Get(), m_indexBufferFormat, 0);
	//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//}

	//// Draw model

	//// Setup vertex shader
	//{
	//	MMDVertexShaderCB vsCB;
	//	vsCB.m_wv = wv;
	//	vsCB.m_wvp = wvp;
	//	context->UpdateSubresource(m_mmdVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

	//	// Vertex shader
	//	context->VSSetShader(appContext.m_mmdVS.Get(), nullptr, 0);
	//	ID3D11Buffer* cbs[] = { m_mmdVSConstantBuffer.Get() };
	//	context->VSSetConstantBuffers(0, 1, cbs);
	//}
	//size_t subMeshCount = m_mmdModel->GetSubMeshCount();
	//for (size_t i = 0; i < subMeshCount; i++)
	//{
	//	const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
	//	const auto& mat = m_materials[subMesh.m_materialID];
	//	const auto& mmdMat = mat.m_mmdMat;

	//	if (mat.m_mmdMat.m_alpha == 0)
	//	{
	//		continue;
	//	}

	//	// Pixel shader
	//	context->PSSetShader(appContext.m_mmdPS.Get(), nullptr, 0);

	//	MMDPixelShaderCB psCB;
	//	psCB.m_alpha = mmdMat.m_alpha;
	//	psCB.m_diffuse = mmdMat.m_diffuse;
	//	psCB.m_ambient = mmdMat.m_ambient;
	//	psCB.m_specular = mmdMat.m_specular;
	//	psCB.m_specularPower = mmdMat.m_specularPower;

	//	if (mat.m_texture.m_texture)
	//	{
	//		if (!mat.m_texture.m_hasAlpha)
	//		{
	//			// Use Material Alpha
	//			psCB.m_textureModes.x = 1;
	//		}
	//		else
	//		{
	//			// Use Material Alpha * Texture Alpha
	//			psCB.m_textureModes.x = 2;
	//		}
	//		psCB.m_texMulFactor = mmdMat.m_textureMulFactor;
	//		psCB.m_texAddFactor = mmdMat.m_textureAddFactor;
	//		ID3D11ShaderResourceView* views[] = { mat.m_texture.m_textureView.Get() };
	//		ID3D11SamplerState* samplers[] = { appContext.m_textureSampler.Get() };
	//		context->PSSetShaderResources(0, 1, views);
	//		context->PSSetSamplers(0, 1, samplers);
	//	}
	//	else
	//	{
	//		psCB.m_textureModes.x = 0;
	//		ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
	//		ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
	//		context->PSSetShaderResources(0, 1, views);
	//		context->PSSetSamplers(0, 1, samplers);
	//	}

	//	if (mat.m_toonTexture.m_texture)
	//	{
	//		psCB.m_textureModes.y = 1;
	//		psCB.m_toonTexMulFactor = mmdMat.m_toonTextureMulFactor;
	//		psCB.m_toonTexAddFactor = mmdMat.m_toonTextureAddFactor;
	//		ID3D11ShaderResourceView* views[] = { mat.m_toonTexture.m_textureView.Get() };
	//		ID3D11SamplerState* samplers[] = { appContext.m_toonTextureSampler.Get() };
	//		context->PSSetShaderResources(1, 1, views);
	//		context->PSSetSamplers(1, 1, samplers);
	//	}
	//	else
	//	{
	//		psCB.m_textureModes.y = 0;
	//		ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
	//		ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
	//		context->PSSetShaderResources(1, 1, views);
	//		context->PSSetSamplers(1, 1, samplers);
	//	}

	//	if (mat.m_spTexture.m_texture)
	//	{
	//		if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
	//		{
	//			psCB.m_textureModes.z = 1;
	//		}
	//		else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
	//		{
	//			psCB.m_textureModes.z = 2;
	//		}
	//		psCB.m_sphereTexMulFactor = mmdMat.m_spTextureMulFactor;
	//		psCB.m_sphereTexAddFactor = mmdMat.m_spTextureAddFactor;
	//		ID3D11ShaderResourceView* views[] = { mat.m_spTexture.m_textureView.Get() };
	//		ID3D11SamplerState* samplers[] = { appContext.m_sphereTextureSampler.Get() };
	//		context->PSSetShaderResources(2, 1, views);
	//		context->PSSetSamplers(2, 1, samplers);
	//	}
	//	else
	//	{
	//		psCB.m_textureModes.z = 0;
	//		ID3D11ShaderResourceView* views[] = { appContext.m_dummyTextureView.Get() };
	//		ID3D11SamplerState* samplers[] = { appContext.m_dummySampler.Get() };
	//		context->PSSetShaderResources(2, 1, views);
	//		context->PSSetSamplers(2, 1, samplers);
	//	}

	//	psCB.m_lightColor = appContext.m_lightColor;
	//	glm::vec3 lightDir = appContext.m_lightDir;
	//	glm::mat3 viewMat = glm::mat3(appContext.m_viewMat);
	//	lightDir = viewMat * lightDir;
	//	psCB.m_lightDir = lightDir;

	//	context->UpdateSubresource(m_mmdPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);
	//	ID3D11Buffer* pscbs[] = { m_mmdPSConstantBuffer.Get() };
	//	context->PSSetConstantBuffers(1, 1, pscbs);

	//	if (mmdMat.m_bothFace)
	//	{
	//		context->RSSetState(appContext.m_mmdBothFaceRS.Get());
	//	}
	//	else
	//	{
	//		context->RSSetState(appContext.m_mmdFrontFaceRS.Get());
	//	}

	//	context->OMSetBlendState(appContext.m_mmdBlendState.Get(), nullptr, 0xffffffff);

	//	context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	//}

	//{
	//	ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr };
	//	ID3D11SamplerState* samplers[] = { nullptr, nullptr, nullptr };
	//	context->PSSetShaderResources(0, 3, views);
	//	context->PSSetSamplers(0, 3, samplers);
	//}

	//// Draw edge

	//// Setup input assembler
	//{
	//	context->IASetInputLayout(appContext.m_mmdEdgeInputLayout.Get());
	//}

	//// Setup vertex shader (VSData)
	//{
	//	MMDEdgeVertexShaderCB vsCB;
	//	vsCB.m_wv = wv;
	//	vsCB.m_wvp = wvp;
	//	vsCB.m_screenSize = glm::vec2(float(appContext.m_screenWidth), float(appContext.m_screenHeight));
	//	context->UpdateSubresource(m_mmdEdgeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

	//	// Vertex shader
	//	context->VSSetShader(appContext.m_mmdEdgeVS.Get(), nullptr, 0);
	//	ID3D11Buffer* cbs[] = { m_mmdEdgeVSConstantBuffer.Get() };
	//	context->VSSetConstantBuffers(0, 1, cbs);
	//}

	//for (size_t i = 0; i < subMeshCount; i++)
	//{
	//	const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
	//	const auto& mat = m_materials[subMesh.m_materialID];
	//	const auto& mmdMat = mat.m_mmdMat;

	//	if (!mmdMat.m_edgeFlag)
	//	{
	//		continue;
	//	}
	//	if (mmdMat.m_alpha == 0.0f)
	//	{
	//		continue;
	//	}

	//	// Edge size constant buffer
	//	{
	//		MMDEdgeSizeVertexShaderCB vsCB;
	//		vsCB.m_edgeSize = mmdMat.m_edgeSize;
	//		context->UpdateSubresource(m_mmdEdgeSizeVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

	//		ID3D11Buffer* cbs[] = { m_mmdEdgeSizeVSConstantBuffer.Get() };
	//		context->VSSetConstantBuffers(1, 1, cbs);
	//	}

	//	// Pixel shader
	//	context->PSSetShader(appContext.m_mmdEdgePS.Get(), nullptr, 0);
	//	{
	//		MMDEdgePixelShaderCB psCB;
	//		psCB.m_edgeColor = mmdMat.m_edgeColor;
	//		context->UpdateSubresource(m_mmdEdgePSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

	//		ID3D11Buffer* pscbs[] = { m_mmdEdgePSConstantBuffer.Get() };
	//		context->PSSetConstantBuffers(2, 1, pscbs);
	//	}

	//	context->RSSetState(appContext.m_mmdEdgeRS.Get());

	//	context->OMSetBlendState(appContext.m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

	//	context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	//}

	//// Draw ground shadow

	//// Setup input assembler
	//{
	//	context->IASetInputLayout(appContext.m_mmdGroundShadowInputLayout.Get());
	//}

	//// Setup vertex shader (VSData)
	//{
	//	auto plane = glm::vec4(0, 1, 0, 0);
	//	auto light = -appContext.m_lightDir;
	//	auto shadow = glm::mat4(1);

	//	shadow[0][0] = plane.y * light.y + plane.z * light.z;
	//	shadow[0][1] = -plane.x * light.y;
	//	shadow[0][2] = -plane.x * light.z;
	//	shadow[0][3] = 0;

	//	shadow[1][0] = -plane.y * light.x;
	//	shadow[1][1] = plane.x * light.x + plane.z * light.z;
	//	shadow[1][2] = -plane.y * light.z;
	//	shadow[1][3] = 0;

	//	shadow[2][0] = -plane.z * light.x;
	//	shadow[2][1] = -plane.z * light.y;
	//	shadow[2][2] = plane.x * light.x + plane.y * light.y;
	//	shadow[2][3] = 0;

	//	shadow[3][0] = -plane.w * light.x;
	//	shadow[3][1] = -plane.w * light.y;
	//	shadow[3][2] = -plane.w * light.z;
	//	shadow[3][3] = plane.x * light.x + plane.y * light.y + plane.z * light.z;

	//	auto wsvp = dxMat * proj * view * shadow * world;

	//	MMDGroundShadowVertexShaderCB vsCB;
	//	vsCB.m_wvp = wsvp;
	//	context->UpdateSubresource(m_mmdGroundShadowVSConstantBuffer.Get(), 0, nullptr, &vsCB, 0, 0);

	//	// Vertex shader
	//	context->VSSetShader(appContext.m_mmdGroundShadowVS.Get(), nullptr, 0);
	//	ID3D11Buffer* cbs[] = { m_mmdGroundShadowVSConstantBuffer.Get() };
	//	context->VSSetConstantBuffers(0, 1, cbs);
	//}

	//// Setup state
	//{
	//	context->RSSetState(appContext.m_mmdGroundShadowRS.Get());
	//	context->OMSetBlendState(appContext.m_mmdGroundShadowBlendState.Get(), nullptr, 0xffffffff);
	//	context->OMSetDepthStencilState(appContext.m_mmdGroundShadowDSS.Get(), 0x01);
	//}

	//for (size_t i = 0; i < subMeshCount; i++)
	//{
	//	const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
	//	const auto& mat = m_materials[subMesh.m_materialID];
	//	const auto& mmdMat = mat.m_mmdMat;

	//	if (!mmdMat.m_groundShadow)
	//	{
	//		continue;
	//	}
	//	if (mmdMat.m_alpha == 0.0f)
	//	{
	//		continue;
	//	}

	//	// Pixel shader
	//	context->PSSetShader(appContext.m_mmdGroundShadowPS.Get(), nullptr, 0);
	//	{
	//		MMDGroundShadowPixelShaderCB psCB;
	//		psCB.m_shadowColor = glm::vec4(0.4f, 0.2f, 0.2f, 0.7f);
	//		context->UpdateSubresource(m_mmdGroundShadowPSConstantBuffer.Get(), 0, nullptr, &psCB, 0, 0);

	//		ID3D11Buffer* pscbs[] = { m_mmdGroundShadowPSConstantBuffer.Get() };
	//		context->PSSetConstantBuffers(1, 1, pscbs);
	//	}

	//	context->OMSetBlendState(appContext.m_mmdEdgeBlendState.Get(), nullptr, 0xffffffff);

	//	context->DrawIndexed(subMesh.m_vertexCount, subMesh.m_beginIndex, 0);
	//}
}
