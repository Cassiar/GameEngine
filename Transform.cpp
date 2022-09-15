#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
	SetPosition(0, 0, 0);
	SetRotation(0, 0, 0);
	SetScale(1, 1, 1);
	RecalcNormals();

	XMStoreFloat4x4(&m_m4WorldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_m4WorldInverseTranspose, XMMatrixIdentity());
	m_bRecalcWorld = false;
	m_pParent = nullptr;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	XMVECTOR pos = XMLoadFloat3(&m_v3Position);
	XMVECTOR offset = XMVectorSet(x, y, z, 0);
	XMStoreFloat3(&m_v3Position, pos + offset);

	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::MoveRelative(float x, float y, float z)
{
	XMVECTOR moveVec = XMVectorSet(x, y, z, 0);

	XMVECTOR rotatedVec = XMVector3Rotate(
		moveVec,
		XMLoadFloat4(&m_v4RotationQuat)
	);

	//Add and store new rotated vector
	XMVECTOR pos = XMLoadFloat3(&m_v3Position);
	XMStoreFloat3(&m_v3Position, pos + rotatedVec);

	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::Rotate(float p, float y, float r)
{
	XMVECTOR rot = XMLoadFloat3(&m_v3EulerAngles);
	XMVECTOR offset = XMVectorSet(p, y, r, 0);
	XMStoreFloat3(&m_v3EulerAngles, rot + offset);

	m_bRecalcWorld = true;
	m_bRecalcNormals = true;
	MarkChildrenDirty();
}

void Transform::Scale(float x, float y, float z)
{
	XMVECTOR scale = XMLoadFloat3(&m_v3Scale);
	XMVECTOR offset = XMVectorSet(x, y, z, 0);
	XMStoreFloat3(&m_v3Scale, scale * offset);

	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::Scale(float scalar)
{
	XMVECTOR scale = XMLoadFloat3(&m_v3Scale);
	XMStoreFloat3(&m_v3Scale, scale * scalar);

	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::SetPosition(float x, float y, float z)
{
	m_v3Position.x = x;
	m_v3Position.y = y;
	m_v3Position.z = z;

	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::SetPosition(DirectX::XMFLOAT3 newPos)
{
	m_v3Position = newPos;
	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::SetRotation(float p, float y, float r)
{
	m_v3EulerAngles.x = p;
	m_v3EulerAngles.y = y;
	m_v3EulerAngles.z = r;

	m_bRecalcWorld = true;
	m_bRecalcNormals = true;
	MarkChildrenDirty();
}

void Transform::SetRotation(DirectX::XMFLOAT3 newRot)
{
	m_v3EulerAngles = newRot;
	m_bRecalcWorld = true;
	m_bRecalcNormals = true;
	MarkChildrenDirty();
}

void Transform::SetScale(float x, float y, float z)
{
	m_v3Scale.x = x;
	m_v3Scale.y = y;
	m_v3Scale.z = z;

	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::SetScale(DirectX::XMFLOAT3 newScale)
{
	m_v3Scale = newScale;
	m_bRecalcWorld = true;
	MarkChildrenDirty();
}

void Transform::SetTransformsFromMatrix(DirectX::XMFLOAT4X4 newWorldMatrix)
{
	XMVECTOR position;
	//Gets stored as a quaternion
	XMVECTOR rotation;
	XMVECTOR scale;

	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&newWorldMatrix));

	//Makes sure to store both the Euler and quaternion
	XMStoreFloat4(&m_v4RotationQuat, rotation);
	m_v3EulerAngles = QuatToEuler(m_v4RotationQuat);

	XMStoreFloat3(&m_v3Position, position);
	XMStoreFloat3(&m_v3Scale, scale);

	m_bRecalcWorld = true;
	m_bRecalcNormals = true;
	MarkChildrenDirty();
}

DirectX::XMFLOAT3 Transform::GetForward()
{
	RecalcNormals();
	return m_v3Forward;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
	RecalcNormals();
	return m_v3Up;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
	RecalcNormals();
	return m_v3Right;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	RecalcWorldAndInverseTranspose();
	return m_m4WorldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	RecalcWorldAndInverseTranspose();
	return m_m4WorldInverseTranspose;
}

void Transform::AddChild(Transform* child, bool makeRelative)
{
	if (!child)
		return;

	//Makes sure child isn't already in the list 
	if (GetChildIndex(child) != -1)
		return;

	//Makes sure the child doesn't move when after being made relative to the parent transform
	if (makeRelative)
	{
		XMFLOAT4X4 tempWorld = GetWorldMatrix();
		XMFLOAT4X4 tempChildWorld = child->GetWorldMatrix();
		XMMATRIX parentWorld = XMLoadFloat4x4(&tempWorld);
		XMMATRIX childWorld = XMLoadFloat4x4(&tempChildWorld);

		XMMATRIX pWorldInv = XMMatrixInverse(0, parentWorld);
		XMMATRIX relCWorld = childWorld * pWorldInv;

		// Set the child's transform from this new matrix
		XMFLOAT4X4 relativeChildWorld;
		XMStoreFloat4x4(&relativeChildWorld, relCWorld);
		child->SetTransformsFromMatrix(relativeChildWorld);
	}

	m_lChildren.push_back(child);
	child->m_pParent = this;

	child->m_bRecalcWorld = true;
	child->m_bRecalcNormals = true;
	child->MarkChildrenDirty();
}

void Transform::RemoveChild(Transform* child)
{
	if (!child)
		return;

	int childIndex = GetChildIndex(child);
	//Child isn't in list
	if (childIndex == -1)
		return;

	m_lChildren.erase(m_lChildren.begin() + childIndex);
	child->m_pParent = nullptr;
	child->m_bRecalcWorld = true;
	child->m_bRecalcNormals = true;
	child->MarkChildrenDirty();
}

Transform* Transform::RemoveChildByIndex(unsigned int index)
{
	if (index >= m_lChildren.size())
		return nullptr;

	Transform* removedChild = m_lChildren[index];
	m_lChildren.erase(m_lChildren.begin() + index);

	removedChild->m_pParent = nullptr;
	removedChild->m_bRecalcWorld = true;
	removedChild->m_bRecalcNormals = true;
	removedChild->MarkChildrenDirty();

	return removedChild;
}

void Transform::SetParent(Transform* newParent)
{
	if (m_pParent)
		m_pParent->RemoveChild(this);

	//This works because the m_pParent field is set in add child
	if (newParent)
		newParent->AddChild(this);
}

void Transform::MarkChildrenDirty()
{
	for (auto& child : m_lChildren) {
		child->m_bRecalcWorld = true;
		child->m_bRecalcNormals = true;
	}
}

void Transform::RecalcWorldAndInverseTranspose()
{
	if (!m_bRecalcWorld)
		return;

	XMMATRIX translation = XMMatrixTranslation(m_v3Position.x, m_v3Position.y, m_v3Position.z);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_v3EulerAngles.x, m_v3EulerAngles.y, m_v3EulerAngles.z);
	XMMATRIX scale = XMMatrixScaling(m_v3Scale.x, m_v3Scale.y, m_v3Scale.z);

	XMMATRIX worldMat = scale * rotation * translation;

	// Is there a parent?
	if (m_pParent)
	{
		XMFLOAT4X4 parentWorld = m_pParent->GetWorldMatrix();
		worldMat *= XMLoadFloat4x4(&parentWorld);
	}

	XMStoreFloat4x4(&m_m4WorldMatrix, worldMat);
	XMStoreFloat4x4(&m_m4WorldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));
	m_bRecalcWorld = false;
}

void Transform::RecalcNormals()
{
	//Only does work if the normals actually needs to be recalced
	if (!m_bRecalcNormals)
		return;

	XMVECTOR rotationQuat = XMQuaternionRotationRollPitchYaw(m_v3EulerAngles.x, m_v3EulerAngles.y, m_v3EulerAngles.z);
	//Storing rotation quaternion for other use since it's already being calculated here every time
	XMStoreFloat4(&m_v4RotationQuat, rotationQuat);

	//Recalculates Forward Vector
	XMStoreFloat3(&m_v3Forward, XMVector3Rotate(
		XMVectorSet(0, 0, 1, 0),
		rotationQuat));

	//Recalculates Up Vector
	XMStoreFloat3(&m_v3Up, XMVector3Rotate(
		XMVectorSet(0, 1, 0, 0),
		rotationQuat));

	//Recalculates Right Vector
	XMStoreFloat3(&m_v3Right, XMVector3Rotate(
		XMVectorSet(1, 0, 0, 0),
		rotationQuat));

	m_bRecalcNormals = false;
}

XMFLOAT3 Transform::QuatToEuler(XMFLOAT4 quat)
{
	//Converts the quaternion to a rotation matrix then
	//decomposes the matrix's components into Euler

	XMMATRIX tempRotMat = XMMatrixRotationQuaternion(XMLoadFloat4(&quat));

	XMFLOAT4X4 rotMat;
	XMStoreFloat4x4(&rotMat, tempRotMat);

	float pitch = asinf(-rotMat._32);
	float yaw = atan2f(rotMat._31, rotMat._33);
	float roll = atan2f(rotMat._12, rotMat._22);

	return XMFLOAT3(pitch, yaw, roll);
}
