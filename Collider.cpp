#include "Collider.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<Mesh> colliderMesh) 
	: m_colliderMesh(colliderMesh)
{

}


void Collider::CalcAABB() 
{
	std::vector<std::shared_ptr<Vertex>> verts = m_colliderMesh->GetVerticies();

	DirectX::XMVECTOR centerAvg;
	for (const auto& vert : verts)
	{
		XMVECTOR position = XMLoadFloat3(&vert->Position);
		centerAvg += position;
	}
	//centerAvg/=verts

	//XMFLOAT3 center;
	//XMStoreFloat3(&center, centerAvg);

	float xMax = verts[0]->Position.x;
	float xMin = verts[0]->Position.x;
	
	float yMax = verts[0]->Position.y;
	float yMin = verts[0]->Position.y;
	
	float zMax = verts[0]->Position.z;
	float zMin = verts[0]->Position.z;


	for (int i = 1; i < verts.size(); i++)
	{
		XMFLOAT3 currPos = verts[i]->Position;
		
		xMax = currPos.x > xMax ? currPos.x : xMax;
		xMin = currPos.x < xMin ? currPos.x : xMax;

		yMax = currPos.y > yMax ? currPos.y : yMax;
		yMin = currPos.y < yMin ? currPos.y : yMax;

		zMax = currPos.z > zMax ? currPos.z : zMax;
		zMin = currPos.z < zMin ? currPos.z : zMax;
	}
}