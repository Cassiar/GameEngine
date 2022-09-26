#include "Collider.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<Mesh> colliderMesh, std::shared_ptr<Transform> transform)
	: m_objectMesh(colliderMesh),
	m_transform(transform),
	m_pointsDirty(true),
	m_halvesDirty(true)
{
	CalcCenterPoint();
}

//TODO: OPTIMIZEEEEE This func could 100% be optimized better
void Collider::CalcMinMaxPoints() 
{
	// Makes sure we don't call this when it's not needed
	if (!m_pointsDirty) {
		return;
	}

	std::vector<Vertex> verts = m_objectMesh->GetVerticies();

	//Inefficient could probs be better done through a compute shader
	XMVECTOR currPos = XMVector4Transform(XMLoadFloat3(&verts[0].Position), XMLoadFloat4x4(&m_transform->GetWorldMatrix()));
	XMFLOAT3 vecToPush;
	XMStoreFloat3(&vecToPush, currPos);
	l_transformedPositions.push_back(vecToPush);

	XMVECTOR maxVec = currPos;
	XMVECTOR minVec = currPos;

	for (int i = 1; i < verts.size(); i++)
	{
		//See above suggestion
		currPos = XMVector4Transform(XMLoadFloat3(&verts[0].Position), XMLoadFloat4x4(&m_transform->GetWorldMatrix()));
		XMStoreFloat3(&vecToPush, currPos);
		l_transformedPositions.push_back(vecToPush);
		
		maxVec = XMVectorGreater(currPos, maxVec);
		minVec = XMVectorLess(currPos, minVec);
	}

	XMStoreFloat3(&m_maxPoint, maxVec);
	XMStoreFloat3(&m_minPoint, minVec);

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

bool Collider::CheckGJKCollision(const std::shared_ptr<Collider> other) {
	XMVECTOR currSupport = CalcSupport(XMLoadFloat3(&m_maxPoint)) - other->CalcSupport(-XMLoadFloat3(&m_maxPoint));

	std::vector<XMVECTOR> supports;
	supports.push_back(currSupport);

	XMVECTOR currDir = -currSupport;

	for (int i = 0; i < l_transformedPositions.size(); i++) {
		XMVECTOR pointA = CalcSupport(currDir) - other->CalcSupport(-currDir);
		XMVECTOR aDotDir = XMVector3Dot(pointA, currDir);
		float fADotDir;
		XMStoreFloat(&fADotDir, aDotDir);

		if (fADotDir < 0) {
			return false;
		}

		supports.push_back(pointA);

		if (DoSimplex(supports, currDir)) {
			return true;
		}
	}

	return false;
}

//Finds the point furthest along the direction vector provided
XMVECTOR Collider::CalcSupport(const XMVECTOR& direction) {
	XMVECTOR max = XMVector3Dot(XMLoadFloat3(&l_transformedPositions[0]), direction);

	int posIndex = 0;

	for (int i = 1; i < l_transformedPositions.size(); i++) {
		XMVECTOR currDot = XMVector3Dot(XMLoadFloat3(&l_transformedPositions[i]), direction);

		if (XMVector3Greater(currDot, max)) {
			max = currDot;
			posIndex = i;
		}
	}

	return XMLoadFloat3(&l_transformedPositions[posIndex]);
}

//Implementation based on https://www.youtube.com/watch?v=Qupqu1xe7Io

bool Collider::DoSimplex(std::vector<XMVECTOR> supports, DirectX::XMVECTOR& direction) {
	XMVECTOR zeroVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR ao = -supports[1];
	XMVECTOR ab = supports[0] - supports[1];

	XMVECTOR ac;
	XMVECTOR abc;
	if (supports.size() > 2) {
		ac = supports[0] - supports[2];
		abc = XMVector3Cross(ab, ac);
	}

	auto dotEval = [&ao, &zeroVec](XMVECTOR vecToDot) {
		return XMVector3Greater(XMVector3Dot(vecToDot, ao), zeroVec);
	};

	auto abDotCase = [&dotEval, &ao, &ab, &supports, &direction]() {
		if (dotEval(ab)) {
			direction = XMVector3Cross(XMVector3Cross(ab, ao), ab);
		}
		else {
			direction = ao;
			supports.erase(supports.begin() + 1);
		}
	};

	

	switch (supports.size()) {
	case 2:
		abDotCase();
		break;

	case 3:
		// Plane ABC x Vector AC
		if (dotEval(XMVector3Cross(abc, ac))) {

			if (dotEval(ac)) {
				direction = XMVector3Cross(XMVector3Cross(ac, ao), ac);
				supports.erase(supports.begin() + 1);
			}
			else {
				abDotCase();
			}
		}
		else {
			if (dotEval(XMVector3Cross(ab, abc))) {
				abDotCase();
			}
			else {
				if (dotEval(abc)) {
					direction = abc;
				}
				else {
					direction = -abc;
					XMVECTOR temp = supports[1];
					supports[1] = supports[2];
					supports[2] = temp;
				}
			}
		}

		break;

	case 4:

		break;
	}

	return false;
}
