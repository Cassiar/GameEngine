#include "Collider.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<Mesh> colliderMesh, Transform* parentTransform)
	: m_objectMesh(colliderMesh),
	m_pointsDirty(true),
	m_halvesDirty(true),
	m_sphere(nullptr),
	m_cube(nullptr)
{
	m_transform = Transform();
	parentTransform->AddChild(&m_transform);

	CalcCenterPoint();
}

Collider::Collider(std::shared_ptr<Mesh> colliderMesh, Transform* parentTransform, Transform* sphere, Transform* cube)
	: m_objectMesh(colliderMesh),
	m_pointsDirty(true),
	m_halvesDirty(true),
	m_sphere(sphere),
	m_cube(cube)
{
	m_transform = Transform();
	parentTransform->AddChild(&m_transform);

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

	XMFLOAT4 currPos = XMFLOAT4(verts[0].Position.x, verts[0].Position.y, verts[0].Position.z, 1.0f);
	XMFLOAT4X4 worldMat = m_transform.GetParent()->GetWorldMatrix();
	
	XMStoreFloat4(&currPos, XMVector4Transform(XMLoadFloat4(&currPos), XMLoadFloat4x4(&worldMat)));

	l_transformedPositions.clear();
	l_transformedPositions.push_back(currPos);

	//Inefficient could probs be better done through a compute shader
	float xMax = currPos.x;
	float xMin = currPos.x;
				 
	float yMax = currPos.y;
	float yMin = currPos.y;
				 
	float zMax = currPos.z;
	float zMin = currPos.z;
	for (int i = 1; i < verts.size(); i++)
	{
		currPos = XMFLOAT4(verts[i].Position.x, verts[i].Position.y, verts[i].Position.z, 1.0f);
		XMStoreFloat4(&currPos, XMVector4Transform(XMLoadFloat4(&currPos), XMLoadFloat4x4(&worldMat)));
		l_transformedPositions.push_back(currPos);

		xMax = currPos.x > xMax ? currPos.x : xMax;
		xMin = currPos.x < xMin ? currPos.x : xMin;

		yMax = currPos.y > yMax ? currPos.y : yMax;
		yMin = currPos.y < yMin ? currPos.y : yMin;

		zMax = currPos.z > zMax ? currPos.z : zMax;
		zMin = currPos.z < zMin ? currPos.z : zMin;
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
		return;
	}

	CalcMinMaxPoints();

	m_halfWidth =  (m_maxPoint.x - m_minPoint.x) / 2.0f;
	m_halfHeight = (m_maxPoint.y - m_minPoint.y) / 2.0f;
	m_halfDepth =  (m_maxPoint.z - m_minPoint.z) / 2.0f;

	m_halvesDirty = false;
}

void Collider::CalcCenterPoint() {
	CalcHalfDimensions();

	XMFLOAT3 parentPos = m_transform.GetParent()->GetPosition();
	XMFLOAT3 thisPos = m_transform.GetPosition();
	m_centerPoint = XMFLOAT3(parentPos.x + thisPos.x, parentPos.y + thisPos.y, parentPos.z + thisPos.z);
	XMFLOAT3 maxPoint = XMFLOAT3(m_maxPoint.x + thisPos.x, m_maxPoint.y + thisPos.y, m_maxPoint.z + thisPos.z);

	m_preCheckRadiusSquared = powf(maxPoint.x - m_centerPoint.x, 2.0f) + powf(maxPoint.y - m_centerPoint.y, 2.0f) + powf(maxPoint.z - m_centerPoint.z, 2.0f);

	
	if (m_sphere)
	{
		XMFLOAT3 parentScale = m_sphere->GetParent()->GetScale();
		float scale = sqrtf(m_preCheckRadiusSquared);
		
		//m_sphere->SetScale(1.0f, 1.0f, 1.0f);
		m_sphere->SetScale(scale / parentScale.x, scale / parentScale.y, scale / parentScale.z);
	}

	if (m_cube && m_sphere) {
		m_cube->SetPosition(m_sphere->GetParent()->GetPosition());
		m_cube->SetScale(XMFLOAT3(m_halfWidth, m_halfHeight, m_halfDepth));
	}
}

bool Collider::CheckForCollision(const std::shared_ptr<Collider> other, bool overrideSphereCheck /* Default = false */) {
	if (!overrideSphereCheck && !CheckSphereColliding(other))
	{
		return false;
	}

	return !CheckSATCollision(other);//true;// CheckGJKCollision(other);
}

bool Collider::CheckSphereColliding(const std::shared_ptr<Collider> other) {
	//Should be subject to change not a great position to mark this
	m_pointsDirty = m_transform.IsWorldDirty();
	m_halvesDirty = m_pointsDirty;

	//Shouldn't be computationally expensive to do this if we maintain the proper dirty booleans
	CalcCenterPoint();
	other->CalcCenterPoint();

	float centerSquareDist = (float)pow(m_centerPoint.x - other->m_centerPoint.x, 2.0f) + pow(m_centerPoint.y - other->m_centerPoint.y, 2.0f) + pow(m_centerPoint.z - other->m_centerPoint.z, 2.0f);
	if (centerSquareDist <=  (m_preCheckRadiusSquared + other->m_preCheckRadiusSquared)) {
		return true;
	}

	return false;
}

#pragma region SAT collision

// Code edited and re-used from an old DSA2 project which I believe referenced a book that I can't seem to find the name of TODO: Cite the book here
int Collider::CheckSATCollision(const std::shared_ptr<Collider> other) {
	//XMVECTOR thisUp = XMLoadFloat3( &(m_transform.GetParent()->GetUp()));
	//XMVECTOR thisForward = XMLoadFloat3( &(m_transform.GetParent()->GetUp()));
	//XMVECTOR thisRight = XMLoadFloat3( &(m_transform.GetParent()->GetUp()));

	float ra, rb;
	float R[3][3] = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}};
	float AbsR[3][3] = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	//aU and bU are the same as their respective Model Matricies
	XMFLOAT4X4 world = m_transform.GetWorldMatrix();
	XMFLOAT4X4 otherWorld = other->m_transform.GetWorldMatrix();
	XMMATRIX aU = XMMatrixTranspose(XMLoadFloat4x4(&(world)));
	XMMATRIX bU = XMMatrixTranspose(XMLoadFloat4x4(&(otherWorld)));

	// Compute rotation matrix expressing b in a's coordinate frame
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			XMStoreFloat(&R[i][j], XMVector3Dot(aU.r[i], bU.r[j]));

	// Compute translation vector t
	XMVECTOR vecT = XMLoadFloat3(&other->m_centerPoint) - XMLoadFloat3(&m_centerPoint);
	// Bring translation into a�s coordinate frame
	XMFLOAT3 tempDot;
	XMStoreFloat(&tempDot.x, XMVector3Dot(vecT, aU.r[0]));
	XMStoreFloat(&tempDot.y, XMVector3Dot(vecT, aU.r[1]));
	XMStoreFloat(&tempDot.z, XMVector3Dot(vecT, aU.r[2]));
	vecT = XMLoadFloat3(&tempDot);
	XMFLOAT3 vecTLoad;
	XMStoreFloat3(&vecTLoad, vecT);

	float t[3];
	t[0] = vecTLoad.x;
	t[1] = vecTLoad.y;
	t[2] = vecTLoad.z;

	// Compute common subexpressions. Add in an epsilon term to
	// counteract arithmetic errors when two edges are parallel and
	// their cross product is (near) null (see text for details)
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			AbsR[i][j] = abs(R[i][j]) + .0000001f;

	float halfWidths[3] = { m_halfWidth, m_halfHeight, m_halfDepth };
	float otherHalfWidths[3] = { other->m_halfWidth, other->m_halfHeight, other->m_halfDepth };
	// Test axes L = A0, L = A1, L = A2
	for (int i = 0; i < 3; i++) {
		ra = halfWidths[i];
		rb = otherHalfWidths[0] * AbsR[i][0] + otherHalfWidths[1] * AbsR[i][1] + otherHalfWidths[2] * AbsR[i][2];
		if (abs(t[i]) > ra + rb) {
			return 1 + i;
		}
	}

	// Test axes L = B0, L = B1, L = B2
	for (int i = 0; i < 3; i++) {
		ra = halfWidths[0] * AbsR[0][i] + halfWidths[1] * AbsR[1][i] + halfWidths[2] * AbsR[2][i];
		rb = otherHalfWidths[i];
		if (abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb)
			return 4 + i;
	}
	// Test axis L = A0 x B0
	ra =halfWidths[1] * AbsR[2][0] +halfWidths[2] * AbsR[1][0];
	rb = otherHalfWidths[1] * AbsR[0][2] + otherHalfWidths[2] * AbsR[0][1];
	if (abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb)
		return eSATResults::SAT_AXxBX;

	// Test axis L = A0 x B1
	ra =halfWidths[1] * AbsR[2][1] +halfWidths[2] * AbsR[1][1];
	rb = otherHalfWidths[0] * AbsR[0][2] + otherHalfWidths[2] * AbsR[0][0];
	if (abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb)
		return eSATResults::SAT_AXxBY;

	// Test axis L = A0 x B2
	ra =halfWidths[1] * AbsR[2][2] +halfWidths[2] * AbsR[1][2];
	rb = otherHalfWidths[0] * AbsR[0][1] + otherHalfWidths[1] * AbsR[0][0];
	if (abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb)
		return eSATResults::SAT_AXxBZ;

	// Test axis L = A1 x B0
	ra =halfWidths[0] * AbsR[2][0] +halfWidths[2] * AbsR[0][0];
	rb = otherHalfWidths[1] * AbsR[1][2] + otherHalfWidths[2] * AbsR[1][1];
	if (abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb)
		return eSATResults::SAT_AYxBX;

	// Test axis L = A1 x B1
	ra =halfWidths[0] * AbsR[2][1] +halfWidths[2] * AbsR[0][1];
	rb = otherHalfWidths[0] * AbsR[1][2] + otherHalfWidths[2] * AbsR[1][0];
	if (abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb)
		return eSATResults::SAT_AYxBY;

	// Test axis L = A1 x B2
	ra =halfWidths[0] * AbsR[2][2] +halfWidths[2] * AbsR[0][2];
	rb = otherHalfWidths[0] * AbsR[1][1] + otherHalfWidths[1] * AbsR[1][0];
	if (abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb)
		return eSATResults::SAT_AYxBZ;

	// Test axis L = A2 x B0
	ra =halfWidths[0] * AbsR[1][0] +halfWidths[1] * AbsR[0][0];
	rb = otherHalfWidths[1] * AbsR[2][2] + otherHalfWidths[2] * AbsR[2][1];
	if (abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb)
		return eSATResults::SAT_AZxBX;

	// Test axis L = A2 x B1
	ra =halfWidths[0] * AbsR[1][1] +halfWidths[1] * AbsR[0][1];
	rb = otherHalfWidths[0] * AbsR[2][2] + otherHalfWidths[2] * AbsR[2][0];
	if (abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb)
		return eSATResults::SAT_AZxBY;

	// Test axis L = A2 x B2
	ra =halfWidths[0] * AbsR[1][2] +halfWidths[1] * AbsR[0][2];
	rb = otherHalfWidths[0] * AbsR[2][1] + otherHalfWidths[1] * AbsR[2][0];
	if (abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb)
		return eSATResults::SAT_AZxBZ;

	//there is no axis test that separates this two objects
	return eSATResults::SAT_NONE;
}

#pragma endregion


#pragma region GJK collision

bool Collider::CheckGJKCollision(const std::shared_ptr<Collider> other) {
	std::vector<XMVECTOR> supports;
	XMVECTOR currSupport = CalcSupport(XMVector3Normalize(XMLoadFloat3(&m_maxPoint))) - other->CalcSupport(-XMVector3Normalize(XMLoadFloat3(&m_maxPoint)));
	supports.push_back(currSupport);

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

		supports.push_back(pointA);

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
	XMVECTOR max = XMVector3Dot(XMLoadFloat3(&l_transformedCubeVerts[0]), direction);

	int posIndex = 0;

	for (int i = 1; i < l_transformedCubeVerts.size(); i++) {
		XMVECTOR currDot = XMVector3Dot(XMLoadFloat3(&l_transformedCubeVerts[i]), direction);

		if (XMVector3Greater(currDot, max)) {
			max = currDot;
			posIndex = i;
		}
	}

	return XMLoadFloat3(&l_transformedCubeVerts[posIndex]);
}

//Implementation based on https://www.youtube.com/watch?v=Qupqu1xe7Io

bool Collider::DoSimplex(std::vector<XMVECTOR>& supports, DirectX::XMVECTOR& direction) {
	XMVECTOR ao = -supports[0];
	XMVECTOR zeroVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);


	static auto dotEval = [&ao, &zeroVec](XMVECTOR vecToDot) {
		return XMVector3Greater(XMVector3Dot(vecToDot, ao), zeroVec);
	};

	static auto lineCase = [&ao, &supports, &direction]() {
		XMVECTOR ab = supports[1] - supports[0];

		if (dotEval(ab)) {
			direction = XMVector3Cross(XMVector3Cross(ab, ao), ab);
		}
		else {
			direction = ao;
			supports.erase(supports.begin() + 1);
		}

		return false;
	};

	static auto triCase = [&ao, &supports, &direction]() {
		XMVECTOR ab = supports[1] - supports[0];
		XMVECTOR ac = supports[2] - supports[0];
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
					XMVECTOR temp = supports[1];
					supports[1] = supports[2];
					supports[2] = temp;
				}
			}
		}

		return false;
	};

	static auto quadCase = [&ao, &supports, &direction]() {
		XMVECTOR ab = supports[1] - supports[0];
		XMVECTOR ac = supports[2] - supports[0];
		XMVECTOR ad = supports[3] - supports[0];

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
			XMVECTOR temp = supports[1];
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