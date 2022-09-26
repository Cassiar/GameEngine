#include "Collider.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<Mesh> colliderMesh)
	: m_objectMesh(colliderMesh),
	m_pointsDirty(true),
	m_halvesDirty(true)
{
	CalcCenterPoint();
}


void Collider::CalcMinMaxPoints() 
{
	// Makes sure we don't call this when it's not needed
	if (!m_pointsDirty) {
		return;
	}

	std::vector<std::shared_ptr<Vertex>> verts = m_objectMesh->GetVerticies();

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

	XMStoreFloat3(&m_maxPoint, XMVectorSet(xMax, yMax, zMax, 0.0f));
	XMStoreFloat3(&m_minPoint, XMVectorSet(xMin, yMin, zMin, 0.0f));

	m_pointsDirty = false;
}

void Collider::CalcHalfDimensions() {
	if (!m_halvesDirty) {
		return;
	}

	CalcMinMaxPoints();

	m_halfWidth =  abs(m_maxPoint.x - m_minPoint.x) / 2.0f;
	m_halfHeight = abs(m_maxPoint.y - m_minPoint.y) / 2.0f;
	m_halfDepth =  abs(m_maxPoint.z - m_minPoint.z) / 2.0f;

	m_preCheckRadiusSquared = pow(m_halfWidth, 2) + pow(m_halfHeight, 2) + pow(m_halfDepth, 2);

	CalcCenterPoint();

	m_halvesDirty = false;
}

void Collider::CalcCenterPoint() {
	CalcHalfDimensions();

	XMStoreFloat3(&m_centerPoint, XMVectorSet(m_maxPoint.x - m_halfWidth, m_maxPoint.y - m_halfHeight, m_maxPoint.z - m_halfDepth, 0.0f));
}

bool Collider::CheckForCollision(const std::shared_ptr<Collider> other) {
	//Shouldn't be computationally expensive to do this if we maintain the proper dirty booleans
	CalcCenterPoint();
	other->CalcCenterPoint();

	float centerSquareDist = pow(m_centerPoint.x - other->m_centerPoint.x, 2.0f) + pow(m_centerPoint.y - other->m_centerPoint.y, 2) + pow(m_centerPoint.z - other->m_centerPoint.z, 2);
	if (m_preCheckRadiusSquared + other->m_preCheckRadiusSquared < centerSquareDist) {
		return false;
	}
}