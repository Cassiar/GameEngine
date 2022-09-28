#include "Collider.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<Mesh> colliderMesh, Transform* transform)
	: m_objectMesh(colliderMesh),
	m_transform(transform),
	m_debugTransform(nullptr),
	m_pointsDirty(true),
	m_halvesDirty(true)
{
	CalcCenterPoint();
}

Collider::Collider(std::shared_ptr<Mesh> colliderMesh, Transform* transform, Transform* debugTransform)
	: m_objectMesh(colliderMesh),
	m_transform(transform),
	m_debugTransform(debugTransform),
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

	XMFLOAT4 tempPos = XMFLOAT4(verts[0].Position.x + m_transform->GetPosition().x, verts[0].Position.y + m_transform->GetPosition().y, verts[0].Position.z + m_transform->GetPosition().z, 1.0f);
	XMFLOAT4X4 worldMat = m_transform->GetWorldMatrix();
	XMStoreFloat4(&tempPos, XMVector4Transform(XMLoadFloat4(&tempPos), XMLoadFloat4x4(&worldMat)));
	l_transformedPositions.clear();
	l_transformedPositions.push_back(tempPos);

	//Inefficient could probs be better done through a compute shader
	float xMax = tempPos.x;
	float xMin = tempPos.x;
				 
	float yMax = tempPos.y;
	float yMin = tempPos.y;
				 
	float zMax = tempPos.z;
	float zMin = tempPos.z;
	for (int i = 1; i < verts.size(); i++)
	{
		XMFLOAT4 currPos = XMFLOAT4(verts[i].Position.x + m_transform->GetPosition().x, verts[i].Position.y + m_transform->GetPosition().y, verts[i].Position.z + m_transform->GetPosition().z, 1.0f);
		XMStoreFloat4(&currPos, XMVector4Transform(XMLoadFloat4(&currPos), XMLoadFloat4x4(&worldMat)));
		l_transformedPositions.push_back(currPos);

		xMax = currPos.x > xMax ? currPos.x : xMax;
		xMin = currPos.x < xMin ? currPos.x : xMax;

		yMax = currPos.y > yMax ? currPos.y : yMax;
		yMin = currPos.y < yMin ? currPos.y : yMax;

		zMax = currPos.z > zMax ? currPos.z : zMax;
		zMin = currPos.z < zMin ? currPos.z : zMax;
	}

	XMStoreFloat3(&m_maxPoint, XMVectorSet(xMax, yMax, zMax, 1.0f));
	XMStoreFloat3(&m_minPoint, XMVectorSet(xMin, yMin, zMin, 1.0f));

	l_transformedCubeVerts.clear();
	l_transformedCubeVerts.push_back(m_maxPoint);
	l_transformedCubeVerts.push_back(m_minPoint);
	l_transformedCubeVerts.push_back(XMFLOAT3(xMax, yMax, zMin));
	l_transformedCubeVerts.push_back(XMFLOAT3(xMin, yMax, zMax));
	l_transformedCubeVerts.push_back(XMFLOAT3(xMax, yMin, zMax));
	l_transformedCubeVerts.push_back(XMFLOAT3(xMax, yMin, zMin));
	l_transformedCubeVerts.push_back(XMFLOAT3(xMin, yMax, zMin));
	l_transformedCubeVerts.push_back(XMFLOAT3(xMin, yMin, zMax));

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

	XMStoreFloat3(&m_centerPoint, XMVectorSet(m_maxPoint.x - m_halfWidth, m_maxPoint.y - m_halfHeight, m_maxPoint.z - m_halfDepth, 1.0f));
	if (m_debugTransform)
	{
		m_debugTransform->SetPosition(m_centerPoint);
		m_debugTransform->SetScale(m_halfWidth, m_halfHeight, m_halfDepth);
	}
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

	return true; //CheckGJKCollision(other);
}

#pragma region GJK collision

bool Collider::CheckGJKCollision(const std::shared_ptr<Collider> other) {
	//CalcCenterPoint();
	//other->CalcCenterPoint();

	std::vector<XMVECTOR*> supports;
	XMVECTOR currSupport = CalcSupport(XMVector3Normalize(XMLoadFloat3(&m_maxPoint))) - other->CalcSupport(-XMVector3Normalize(XMLoadFloat3(&m_maxPoint)));
	supports.push_back(&currSupport);

	XMVECTOR negated = XMVectorNegate(currSupport);
	XMVECTOR currDir = negated;

	float fADotDir = 0.0f;

	while (true) {
		XMVECTOR pointA = CalcSupport(currDir) - other->CalcSupport(-(currDir));
		XMVECTOR aDotDir = XMVector3Dot(pointA, currDir);

		XMStoreFloat(&fADotDir, aDotDir);

		if (fADotDir <= 0) {
			return false;
		}

		supports.push_back(&pointA);

		XMFLOAT3 directionPrint;
		XMStoreFloat3(&directionPrint, currDir);

		if (DoSimplex(supports, currDir)) {
			return true;
		}
	}

	return false;
}

//Finds the point furthest along the direction vector provided
XMVECTOR Collider::CalcSupport(const XMVECTOR& direction) {
	XMVECTOR max = XMVector4Dot(XMLoadFloat4(&l_transformedPositions[0]), direction);

	int posIndex = 0;

	for (int i = 1; i < l_transformedPositions.size(); i++) {
		XMVECTOR currDot = XMVector4Dot(XMLoadFloat4(&l_transformedPositions[i]), direction);

		if (XMVector3Greater(currDot, max)) {
			max = currDot;
			posIndex = i;
		}
	}

	return XMLoadFloat4(&l_transformedPositions[posIndex]);
}

//Implementation based on https://www.youtube.com/watch?v=Qupqu1xe7Io

bool Collider::DoSimplex(std::vector<XMVECTOR*>& supports, DirectX::XMVECTOR& direction) {
	XMVECTOR ao = -*supports[0];
	XMVECTOR zeroVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);


	auto dotEval = [&ao, &zeroVec](XMVECTOR vecToDot) {
		return XMVector3Greater(XMVector3Dot(vecToDot, ao), zeroVec);
	};

	auto lineCase = [&dotEval, &ao, &supports, &direction]() {
		XMVECTOR ab = *supports[1] - *supports[0];

		if (dotEval(ab)) {
			direction = XMVector3Cross(XMVector3Cross(ab, ao), ab);
		}
		else {
			direction = ao;
			supports.erase(supports.begin() + 1);
		}

		return false;
	};

	auto triCase = [&dotEval, &lineCase, &ao, &supports, &direction]() {
		XMVECTOR ab = *supports[1] - *supports[0];
		XMVECTOR ac = *supports[2] - *supports[0];
		XMVECTOR abc = XMVector3Cross(ab, ac);

		// Plane ABC x Vector AC
		if (dotEval(XMVector3Cross(abc, ac))) {

			if (dotEval(ac)) {
				direction = XMVector3Cross(XMVector3Cross(ac, ao), ac);
				supports.erase(supports.begin() + 1);
			}
			else {
				return lineCase();
			}
		}
		else {
			if (dotEval(XMVector3Cross(ab, abc))) {
				return lineCase();
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

		return false;
	};

	auto quadCase = [&dotEval, &triCase, &ao, &supports, &direction]() {
		XMVECTOR ab = *supports[1] - *supports[0];
		XMVECTOR ac = *supports[2] - *supports[0];
		XMVECTOR ad = *supports[3] - *supports[0];

		XMVECTOR abc = XMVector3Cross(ab, ac);
		XMVECTOR acd = XMVector3Cross(ac, ad);
		XMVECTOR adb = XMVector3Cross(ad, ab);

		if (dotEval(abc)) {
			supports.erase(supports.begin() + 3);
			return triCase();
		}

		if (dotEval(acd)) {
			supports.erase(supports.begin() + 1);
			return triCase();
		}

		if (dotEval(adb)) {
			supports.erase(supports.begin() + 2);
			XMVECTOR* temp = supports[1];
			supports[1] = supports[2];
			supports[2] = temp;
			return triCase();
		}

		return true;
	};

	switch (supports.size()) {
	case 2:
		return lineCase();

	case 3:
		return triCase();

	case 4:
		return quadCase();

	default:
		break;
	}

	return false;
}

#pragma endregion