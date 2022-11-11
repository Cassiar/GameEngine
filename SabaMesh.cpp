#include "SabaMesh.h"

using namespace DirectX;

SabaMesh::SabaMesh(const char* path, const char* texpath, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context)
 : Mesh(in_context) {
	this->isPmx = true;
	model = std::make_shared<saba::PMXModel>();
	model->Load(path, texpath);

	this->context = in_context;

	//convert saba verts to our verts
	for (int i = 0; i < model->GetVertexCount(); i++) {
		Vertex temp = {};
		temp.Position = XMFLOAT3(model->GetPositions()[i][0], model->GetPositions()[i][1], model->GetPositions()[i][2]);
		temp.Normal = XMFLOAT3(model->GetNormals()[i][0], model->GetNormals()[i][1], model->GetNormals()[i][2]);
		temp.UVCoord = XMFLOAT2(model->GetUVs()[i][0], model->GetUVs()[i][1]);
		temp.Tangent = {};
		m_verts.push_back(temp);
		//printf("i: %d \tU: %f, \tV: %f\n", i,temp.UVCoord.x, temp.UVCoord.y);
	}
	numIndices = model->GetIndexCount();

	//create the buffers and send to GPU
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex) * model->GetVertexCount();       // number of vertices in the buffer
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Tells DirectX this is a vertex buffer
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialVertexData = {};
	initialVertexData.pSysMem = &m_verts[0];

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&vbd, &initialVertexData, vertBuf.GetAddressOf());

	// Create the INDEX BUFFER description 
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = UINT(model->GetIndexElementSize() * model->GetIndexCount());
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	// Create the proper struct to hold the initial index data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialIndexData = {};
	initialIndexData.pSysMem = model->GetIndices();

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&ibd, &initialIndexData, indexBuf.GetAddressOf());

	if (model->GetIndexElementSize() == 1)
	{
		format = DXGI_FORMAT_R8_UINT;
	}
	else if (model->GetIndexElementSize() == 2)
	{
		format = DXGI_FORMAT_R16_UINT;
	}
	else if (model->GetIndexElementSize() == 4)
	{
		format = DXGI_FORMAT_R32_UINT;
	}
}

void SabaMesh::Draw()
{
	Mesh::Draw();

	size_t numSubMeshes = model->GetSubMeshCount();
	for (int i = 0; i < numSubMeshes; i++) {
		const auto& subMesh = model->GetSubMeshes()[i];
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, vertBuf.GetAddressOf(), &stride, &offset);
		//if (isPmx) {
		//	context->IASetIndexBuffer(indexBuf.Get(), format, 0);
		//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//}
		//else {
		//	context->IASetIndexBuffer(indexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
		//}
		context->IASetIndexBuffer(indexBuf.Get(), format, 0);
		// Finally do the actual drawing
		// Once per object
		context->DrawIndexed(
			subMesh.m_vertexCount,     // The number of indices to use (we could draw a subset if we wanted)
			subMesh.m_beginIndex,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}
}
