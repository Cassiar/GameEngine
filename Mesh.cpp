#include "Mesh.h"
#include <fstream>
#include <vector>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <Saba/Model/MMD/PMXModel.h>

using namespace DirectX;

Mesh::Mesh(Vertex* verts, unsigned int numVerts, unsigned int* indices, unsigned int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
    this->numIndices = numIndices;
    this->context = context;
	
	CalculateTangents(verts, numVerts, indices, numIndices);
	CreateBuffers(verts, numVerts, indices, device);
}

Mesh::Mesh(const char* path, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	this->context = context;

	//from open asset importer https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html
	// Create an instance of the Importer class
	Assimp::Importer importer;

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.
	const aiScene* scene = importer.ReadFile(path,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_MakeLeftHanded | //assimp imports in right hand space but directX uses left handed
		aiProcess_FlipWindingOrder | //default is CCW, we want CW
		aiProcess_FlipUVs //flip the uv order to match our file format
	);

	//// If the import failed, report it
	if (nullptr == scene) {
		//DoTheErrorLogging(importer.GetErrorString());
		return;
	}

	// Now we can access the file's contents.
	//DoTheSceneProcessing(scene);
	std::vector<Vertex> verts;		// Verts we're assembling
	std::vector<UINT> indices;		// Indices of these verts
	int vertCounter = 0;			// Count of vertices
	int indexCounter = 0;			// Count of indices

	//should only be one mesh
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* aiMesh = scene->mMeshes[i];
		//aiVector3D* aiVecs = scene->mMeshes[i]->mVertices;
		//aiVector3D** aiTexs = scene->mMeshes[i]->mTextureCoords;
		for (unsigned int j = 0; j < scene->mMeshes[i]->mNumVertices; j++) {
			Vertex temp = {};
			temp.Position = XMFLOAT3(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z);
			//effectively 2d array, first index is what number of texcoords (upto 8) second is this specific one for this specific vert
			temp.UVCoord = XMFLOAT2(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y);
			temp.Normal = XMFLOAT3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);
			temp.Tangent = XMFLOAT3(aiMesh->mTangents[j].x, aiMesh->mTangents[j].y, aiMesh->mTangents[j].z);
			verts.push_back(temp);
			vertCounter++;
			//positions.push_back(XMFLOAT3(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z));
			//uvs.push_back(XMFLOAT2(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y));
			//normals.push_back(XMFLOAT3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z));
			//tangents.push_back(XMFLOAT3(aiMesh->mTangents[j].x, aiMesh->mTangents[j].y, aiMesh->mTangents[j].z));
		}

		//figure out how many faces there are on the mesh
		for (unsigned int j = 0; j < scene->mMeshes[i]->mNumFaces; j++) {
			aiFace aiFace = scene->mMeshes[i]->mFaces[j];
			//add each indice to the list
			for (unsigned int k = 0;k < aiFace.mNumIndices; k++) {

				indexCounter++;
				indices.push_back(aiFace.mIndices[k]);
			}
		}
	}
	// We're done. Everything will be cleaned up by the importer destructor
	//return true;

#pragma region ChrisMeshLoader


	//Provided code

	// Author: Chris Cascioli
	// Purpose: Basic .OBJ 3D model loading, supporting positions, uvs and normals
	// 
	// - You are allowed to directly copy/paste this into your code base
	//   for assignments, given that you clearly cite that this is not
	//   code of your own design.
	//
	// - NOTE: You'll need to #include <fstream>


	// File input object
	//std::ifstream obj(path);

	//// Check for successful open
	//if (!obj.is_open())
	//	return;

	//// Variables used while reading the file
	//std::vector<XMFLOAT3> positions;	// Positions from the file
	//std::vector<XMFLOAT3> normals;		// Normals from the file
	//std::vector<XMFLOAT2> uvs;		// UVs from the file
	//
	//
	//
	//
	//char chars[100];			// String for line reading

	//// Still have data left?
	//while (obj.good())
	//{
	//	// Get the line (100 characters should be more than enough)
	//	obj.getline(chars, 100);

	//	// Check the type of line
	//	if (chars[0] == 'v' && chars[1] == 'n')
	//	{
	//		// Read the 3 numbers directly into an XMFLOAT3
	//		XMFLOAT3 norm;
	//		sscanf_s(
	//			chars,
	//			"vn %f %f %f",
	//			&norm.x, &norm.y, &norm.z);

	//		// Add to the list of normals
	//		normals.push_back(norm);
	//	}
	//	else if (chars[0] == 'v' && chars[1] == 't')
	//	{
	//		// Read the 2 numbers directly into an XMFLOAT2
	//		XMFLOAT2 uv;
	//		sscanf_s(
	//			chars,
	//			"vt %f %f",
	//			&uv.x, &uv.y);

	//		// Add to the list of uv's
	//		uvs.push_back(uv);
	//	}
	//	else if (chars[0] == 'v')
	//	{
	//		// Read the 3 numbers directly into an XMFLOAT3
	//		XMFLOAT3 pos;
	//		sscanf_s(
	//			chars,
	//			"v %f %f %f",
	//			&pos.x, &pos.y, &pos.z);

	//		// Add to the positions
	//		positions.push_back(pos);
	//	}
	//	else if (chars[0] == 'f')
	//	{
	//		// Read the face indices into an array
	//		// NOTE: This assumes the given obj file contains
	//		//  vertex positions, uv coordinates AND normals.
	//		unsigned int i[12];
	//		int numbersRead = sscanf_s(
	//			chars,
	//			"f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
	//			&i[0], &i[1], &i[2],
	//			&i[3], &i[4], &i[5],
	//			&i[6], &i[7], &i[8],
	//			&i[9], &i[10], &i[11]);

	//		// If we only got the first number, chances are the OBJ
	//		// file has no UV coordinates.  This isn't great, but we
	//		// still want to load the model without crashing, so we
	//		// need to re-read a different pattern (in which we assume
	//		// there are no UVs denoted for any of the vertices)
	//		if (numbersRead == 1)
	//		{
	//			// Re-read with a different pattern
	//			numbersRead = sscanf_s(
	//				chars,
	//				"f %d//%d %d//%d %d//%d %d//%d",
	//				&i[0], &i[2],
	//				&i[3], &i[5],
	//				&i[6], &i[8],
	//				&i[9], &i[11]);

	//			// The following indices are where the UVs should 
	//			// have been, so give them a valid value
	//			i[1] = 1;
	//			i[4] = 1;
	//			i[7] = 1;
	//			i[10] = 1;

	//			// If we have no UVs, create a single UV coordinate
	//			// that will be used for all vertices
	//			if (uvs.size() == 0)
	//				uvs.push_back(XMFLOAT2(0, 0));
	//		}

	//		// - Create the verts by looking up
	//		//    corresponding data from vectors
	//		// - OBJ File indices are 1-based, so
	//		//    they need to be adusted
	//		Vertex v1;
	//		v1.Position = positions[i[0] - 1];
	//		v1.UVCoord = uvs[i[1] - 1];
	//		v1.Normal = normals[i[2] - 1];

	//		Vertex v2;
	//		v2.Position = positions[i[3] - 1];
	//		v2.UVCoord = uvs[i[4] - 1];
	//		v2.Normal = normals[i[5] - 1];

	//		Vertex v3;
	//		v3.Position = positions[i[6] - 1];
	//		v3.UVCoord = uvs[i[7] - 1];
	//		v3.Normal = normals[i[8] - 1];

	//		// The model is most likely in a right-handed space,
	//		// especially if it came from Maya.  We want to convert
	//		// to a left-handed space for DirectX.  This means we 
	//		// need to:
	//		//  - Invert the Z position
	//		//  - Invert the normal's Z
	//		//  - Flip the winding order
	//		// We also need to flip the UV coordinate since DirectX
	//		// defines (0,0) as the top left of the texture, and many
	//		// 3D modeling packages use the bottom left as (0,0)

	//		// Flip the UV's since they're probably "upside down"
	//		v1.UVCoord.y = 1.0f - v1.UVCoord.y;
	//		v2.UVCoord.y = 1.0f - v2.UVCoord.y;
	//		v3.UVCoord.y = 1.0f - v3.UVCoord.y;

	//		// Flip Z (LH vs. RH)
	//		v1.Position.z *= -1.0f;
	//		v2.Position.z *= -1.0f;
	//		v3.Position.z *= -1.0f;

	//		// Flip normal's Z
	//		v1.Normal.z *= -1.0f;
	//		v2.Normal.z *= -1.0f;
	//		v3.Normal.z *= -1.0f;

	//		//attempt to add tangents and normal maps in assignment 8 
	//		//calculate tangents source: http://foundationsofgameenginedev.com/FGED2-sample.pdf
	//		/*
	//		XMFLOAT3 e1 = { v2.Position.x - v1.Position.x, v2.Position.y - v1.Position.y, v2.Position.z - v1.Position.z };
	//		XMFLOAT3 e2 = { v3.Position.x - v1.Position.x, v3.Position.y - v1.Position.y, v3.Position.z - v1.Position.z };
	//		//XMStoreFloat3(&e1, XMLoadFloat3(v2.Position) - XMLoadFloat3(v1.Position));
	//		float x1 = v2.UVCoord.x - v1.UVCoord.x;
	//		float x2 = v3.UVCoord.x - v1.UVCoord.x;
	//		float y1 = v2.UVCoord.y - v1.UVCoord.y;
	//		float y2 = v3.UVCoord.y - v1.UVCoord.y;

	//		float r = 1.0f / (x1 * y2 - x2 * y1);
	//		XMFLOAT3 t = { (e1.x * y2 - e2.x * y1)* r, (e1.y * y2 - e2.y * y1)* r, (e1.z * y2 - e2.z * y1)* r };
	//		//XMFLOAT3 b = {};// { (e2.x * x1 - e1.x * x2)* r, (e2.y * x1 - e1.y * x2)* r, (e2.z * x1 - e1.z * x2)* r };

	//		v1.Tangent = t;
	//		v2.Tangent = t;
	//		v3.Tangent = t;

	//		//v1.Bitangent = b;
	//		//v2.Bitangent = b;
	//		//v3.Bitangent = b;
	//		*/

	//		// Add the verts to the vector (flipping the winding order)
	//		verts.push_back(v1);
	//		verts.push_back(v3);
	//		verts.push_back(v2);
	//		vertCounter += 3;

	//		// Add three more indices
	//		indices.push_back(indexCounter); indexCounter += 1;
	//		indices.push_back(indexCounter); indexCounter += 1;
	//		indices.push_back(indexCounter); indexCounter += 1;

	//		// Was there a 4th face?
	//		// - 12 numbers read means 4 faces WITH uv's
	//		// - 8 numbers read means 4 faces WITHOUT uv's
	//		if (numbersRead == 12 || numbersRead == 8)
	//		{
	//			// Make the last vertex
	//			Vertex v4;
	//			v4.Position = positions[i[9] - 1];
	//			v4.UVCoord = uvs[i[10] - 1];
	//			v4.Normal = normals[i[11] - 1];

	//			// Flip the UV, Z pos and normal's Z
	//			v4.UVCoord.y = 1.0f - v4.UVCoord.y;
	//			v4.Position.z *= -1.0f;
	//			v4.Normal.z *= -1.0f;

	//			// Add a whole triangle (flipping the winding order)
	//			verts.push_back(v1);
	//			verts.push_back(v4);
	//			verts.push_back(v3);
	//			vertCounter += 3;

	//			// Add three more indices
	//			indices.push_back(indexCounter); indexCounter += 1;
	//			indices.push_back(indexCounter); indexCounter += 1;
	//			indices.push_back(indexCounter); indexCounter += 1;
	//		}
	//	}
	//}

	//// Close the file and create the actual buffers
	//obj.close();

	#pragma endregion
		//end provided code

		//store number of indices
	numIndices = indexCounter;

	format = DXGI_FORMAT_R32_UINT;
	//CalculateTangents(&verts[0], vertCounter, &indices[0], indexCounter);

	//pass verts and indices on to create buffer
	Mesh::CreateBuffers(&verts[0], vertCounter, &indices[0], device);
}

Mesh::~Mesh()
{
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
    return vertBuf;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
    return indexBuf;
}

unsigned int Mesh::GetIndexCount()
{
    return numIndices;
}

void Mesh::Draw()
{	
	// Set buffers in the input assembler
	//once per object
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

	if (!this->IsPmx())
	{
		context->DrawIndexed(numIndices, 0, 0);
	}
	if (this->IsPmx()) {
		int temp = 0;
	}
}

void Mesh::Draw(Microsoft::WRL::ComPtr<ID3D11RasterizerState> customRast)
{	
	context->RSSetState(customRast.Get());
	
	this->Draw();

	context->RSSetState(0);
}

// Protected constructor for use in SabaMesh.h
Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context)
{
	this->context = in_context;
}

//helper methods
void Mesh::CreateBuffers(Vertex* in_verts, unsigned int numVerts, unsigned int* in_indices, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	for (unsigned int i = 0; i < numVerts; i++) {
		m_verts.push_back(in_verts[i]);
	}

	//create the buffers and send to GPU
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * numVerts;       // number of vertices in the buffer
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Tells DirectX this is a vertex buffer
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialVertexData = {};
	initialVertexData.pSysMem = in_verts;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&vbd, &initialVertexData, vertBuf.GetAddressOf());



	// Create the INDEX BUFFER description 
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * numIndices;	// number of indices in the buffer
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;	// Tells DirectX this is an index buffer
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	// Create the proper struct to hold the initial index data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialIndexData = {};
	initialIndexData.pSysMem = in_indices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&ibd, &initialIndexData, indexBuf.GetAddressOf());
}

// --------------------------------------------------------
// Author: Chris Cascioli
// Purpose: Calculates the tangents of the vertices in a mesh
// 
// - You are allowed to directly copy/paste this into your code base
//   for assignments, given that you clearly cite that this is not
//   code of your own design.
//
// - Code originally adapted from: http://www.terathon.com/code/tangent.html
//   - Updated version now found here: http://foundationsofgameenginedev.com/FGED2-sample.pdf
//   - See listing 7.4 in section 7.5 (page 9 of the PDF)
//
// - Note: For this code to work, your Vertex format must
//         contain an XMFLOAT3 called Tangent
//
// - Be sure to call this BEFORE creating your D3D vertex/index buffers
// --------------------------------------------------------
void Mesh::CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices)
{
	// Reset tangents
	for (int i = 0; i < numVerts; i++)
	{
		verts[i].Tangent = XMFLOAT3(0, 0, 0);
	}

	// Calculate tangents one whole triangle at a time
	for (int i = 0; i < numIndices;)
	{
		// Grab indices and vertices of first triangle
		unsigned int i1 = indices[i++];
		unsigned int i2 = indices[i++];
		unsigned int i3 = indices[i++];
		Vertex* v1 = &verts[i1];
		Vertex* v2 = &verts[i2];
		Vertex* v3 = &verts[i3];

		// Calculate vectors relative to triangle positions
		float x1 = v2->Position.x - v1->Position.x;
		float y1 = v2->Position.y - v1->Position.y;
		float z1 = v2->Position.z - v1->Position.z;

		float x2 = v3->Position.x - v1->Position.x;
		float y2 = v3->Position.y - v1->Position.y;
		float z2 = v3->Position.z - v1->Position.z;

		// Do the same for vectors relative to triangle uv's
		float s1 = v2->UVCoord.x - v1->UVCoord.x;
		float t1 = v2->UVCoord.y - v1->UVCoord.y;

		float s2 = v3->UVCoord.x - v1->UVCoord.x;
		float t2 = v3->UVCoord.y - v1->UVCoord.y;

		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);

		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;

		// Adjust tangents of each vert of the triangle
		v1->Tangent.x += tx;
		v1->Tangent.y += ty;
		v1->Tangent.z += tz;

		v2->Tangent.x += tx;
		v2->Tangent.y += ty;
		v2->Tangent.z += tz;

		v3->Tangent.x += tx;
		v3->Tangent.y += ty;
		v3->Tangent.z += tz;
	}

	// Ensure all of the tangents are orthogonal to the normals
	for (int i = 0; i < numVerts; i++)
	{
		// Grab the two vectors
		XMVECTOR normal = XMLoadFloat3(&verts[i].Normal);
		XMVECTOR tangent = XMLoadFloat3(&verts[i].Tangent);

		// Use Gram-Schmidt orthonormalize to ensure
		// the normal and tangent are exactly 90 degrees apart
		tangent = XMVector3Normalize(
			tangent - normal * XMVector3Dot(normal, tangent));

		// Store the tangent
		XMStoreFloat3(&verts[i].Tangent, tangent);
	}
}