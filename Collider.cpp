#include "Collider.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<Mesh> colliderMesh, Transform* transform)
	: m_objectMesh(colliderMesh),
	m_transform(transform),
	m_pointsDirty(true),
	m_halvesDirty(true)
{
	CalcCenterPoint();
}

//TEMP unused
Collider::~Collider()
{
}

//TODO: OPTIMIZEEEEE This func could 100% be optimized better
void Collider::CalcMinMaxPoints() 
{
	// Makes sure we don't call this when it's not needed
	if (!m_pointsDirty) {
		//return;
	}

	std::vector<Vertex> verts = m_objectMesh->GetVerticies();

	XMFLOAT3 tempPos = verts[0].Position;
	XMFLOAT4X4 worldMat = m_transform->GetWorldMatrix();
	XMStoreFloat3(&tempPos, XMVector4Transform(XMLoadFloat3(&tempPos), XMLoadFloat4x4(&worldMat)));
	l_transformedPositions.push_back(tempPos);

	//Inefficient could probs be better done through a compute shader
	float xMax = verts[0].Position.x;
	float xMin = verts[0].Position.x;
						 
	float yMax = verts[0].Position.y;
	float yMin = verts[0].Position.y;
						 
	float zMax = verts[0].Position.z;
	float zMin = verts[0].Position.z;
	for (int i = 1; i < verts.size(); i++)
	{
		tempPos = verts[i].Position;
		XMFLOAT3 currPos = verts[i].Position;
		XMStoreFloat3(&currPos, XMVector4Transform(XMLoadFloat3(&currPos), XMLoadFloat4x4(&worldMat)));
		l_transformedPositions.push_back(currPos);

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
		//return;
	}

	CalcMinMaxPoints();

	m_halfWidth =  abs(m_maxPoint.x - m_minPoint.x) / 2.0f;
	m_halfHeight = abs(m_maxPoint.y - m_minPoint.y) / 2.0f;
	m_halfDepth =  abs(m_maxPoint.z - m_minPoint.z) / 2.0f;

	m_preCheckRadiusSquared = pow(m_halfWidth, 2) + pow(m_halfHeight, 2) + pow(m_halfDepth, 2);

	m_halvesDirty = false;
}

void Collider::CalcCenterPoint() {
	CalcHalfDimensions();

	XMStoreFloat3(&m_centerPoint, XMVectorSet(m_maxPoint.x - m_halfWidth, m_maxPoint.y - m_halfHeight, m_maxPoint.z - m_halfDepth, 0.0f));
}

bool Collider::CheckForCollision(const std::shared_ptr<Collider> other) {
	//Should be subject to change not a great position to mark this
	m_pointsDirty = m_transform->IsWorldDirty();
	m_halvesDirty = m_pointsDirty;

	//Shouldn't be computationally expensive to do this if we maintain the proper dirty booleans
	CalcCenterPoint();
	other->CalcCenterPoint();

	float centerSquareDist = pow(m_centerPoint.x - other->m_centerPoint.x, 2.0f) + pow(m_centerPoint.y - other->m_centerPoint.y, 2) + pow(m_centerPoint.z - other->m_centerPoint.z, 2);
	if (m_preCheckRadiusSquared + other->m_preCheckRadiusSquared < centerSquareDist) {
		return false;
	}

	return CheckGJKCollision(other);
}

bool Collider::CheckGJKCollision(const std::shared_ptr<Collider> other) {
	CalcCenterPoint();

	std::vector<XMVECTOR*> supports;
	XMVECTOR currSupport = CalcSupport(XMLoadFloat3(&m_maxPoint)) - other->CalcSupport(-XMLoadFloat3(&m_maxPoint));
	supports.push_back(&currSupport);

	XMVECTOR negated = XMVectorNegate(currSupport);
	XMVECTOR* currDir = &negated;

	float fADotDir = 0.0f;

	int i = 0;
	// (fADotDir < 0)
	while(fADotDir >= 0) {
		i++;
		XMVECTOR pointA = CalcSupport(*currDir) - other->CalcSupport(-(*currDir));
		XMVECTOR aDotDir = XMVector3Dot(pointA, *currDir);
		
		XMStoreFloat(&fADotDir, aDotDir);

		if (fADotDir < 0) {
			return false;
		}

		supports.push_back(&pointA);
		
		XMFLOAT3 directionPrint;
		XMStoreFloat3(&directionPrint, *currDir);

		if (DoSimplex(supports, *currDir)) {
			return true;
		}
	}

	return false;
}

//Finds the point furthest along the direction vector provided
XMVECTOR Collider::CalcSupport(const XMVECTOR& direction) {
	XMVECTOR max = XMVector3Dot(XMLoadFloat3(&l_transformedPositions[0]), direction);

	int posIndex = 0;

	for (int i = 1; i < 30 && i < l_transformedPositions.size(); i++) {
		XMVECTOR currDot = XMVector3Dot(XMLoadFloat3(&l_transformedPositions[i]), direction);

		if (XMVector3Greater(currDot, max)) {
			max = currDot;
			posIndex = i;
		}
	}

	return XMLoadFloat3(&l_transformedPositions[posIndex]);
}

//Implementation based on https://www.youtube.com/watch?v=Qupqu1xe7Io

bool Collider::DoSimplex(std::vector<XMVECTOR*>& supports, DirectX::XMVECTOR& direction) {
	XMVECTOR zeroVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR ao = -*supports[1];
	XMVECTOR ab = *supports[0] - *supports[1];

	XMVECTOR ac;
	XMVECTOR abc;
	if (supports.size() > 2) {
		ac = *supports[0] - *supports[2];
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
					XMVECTOR temp = *supports[1];
					supports[1] = supports[2];
					supports[2] = &temp;
				}
			}
		}

		break;

	case 4:
		XMVECTOR ad = *supports[3] - *supports[0];
		XMVECTOR bd = *supports[3] - *supports[1];
		XMVECTOR bc = *supports[2] - *supports[1];

		XMVECTOR abd = XMVector3Cross(ab, ad);
		XMVECTOR acd = XMVector3Cross(ac, ad);
		XMVECTOR bcd = XMVector3Cross(bc, bd);

		if (dotEval(abd)) {
			supports.erase(supports.begin() + 2);

			// Plane ABD x Vector AD
			if (dotEval(XMVector3Cross(abd, ad))) {

				if (dotEval(ad)) {
					direction = XMVector3Cross(XMVector3Cross(ad, ao), ad);
					supports.erase(supports.begin() + 1);
				}
				else {
					abDotCase();
				}
			}
			else {
				if (dotEval(XMVector3Cross(ab, abd))) {
					abDotCase();
				}
				else {
					direction = abd;
				}
			}
		}
		else if (dotEval(acd)) {
			supports.erase(supports.begin() + 1);

			// Plane ACD x Vector AD
			if (dotEval(XMVector3Cross(acd, ad))) {
				if (dotEval(ad)) {
					direction = XMVector3Cross(XMVector3Cross(ad, ao), ad);
					supports.erase(supports.begin() + 1);
				}
				else {
					if (dotEval(ac)) {
						direction = XMVector3Cross(XMVector3Cross(ac, ao), ac);
					}
					else {
						direction = ao;
						supports.erase(supports.begin() + 1);
					}
				}
			}
			else {
				if (dotEval(XMVector3Cross(ac, acd))) {
					if (dotEval(ac)) {
						direction = XMVector3Cross(XMVector3Cross(ac, ao), ac);
					}
					else {
						direction = ao;
						supports.erase(supports.begin() + 1);
					}
				}
				else {
					direction = acd;
				}
			}
		}
		else if (dotEval(bcd)) {
			supports.erase(supports.begin() + 0);
			XMVECTOR bo = -*supports[0];

			// Plane BCD x Vector BD
			if (dotEval(XMVector3Cross(bcd, bd))) {
				if (dotEval(bd)) {
					direction = XMVector3Cross(XMVector3Cross(bd, bo), bd);
					supports.erase(supports.begin() + 1);
				}
				else {
					if (dotEval(bd)) {
						direction = XMVector3Cross(XMVector3Cross(bd, bo), bd);
					}
					else {
						direction = bo;
						supports.erase(supports.begin() + 1);
					}
				}
			}
			else {
				if (dotEval(XMVector3Cross(bd, bcd))) {
					if (dotEval(bc)) {
						direction = XMVector3Cross(XMVector3Cross(bc, bo), bc);
					}
					else {
						direction = bo;
						supports.erase(supports.begin() + 1);
					}
				}
				else {
					direction = bcd;
				}
			}
		}
		else {
			return true;
		}

		break;
	}

	return false;
}
