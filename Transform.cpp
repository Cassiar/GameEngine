#include "Transform.h"

//ok since we're in cpp file.
//also overloads math operators
using namespace DirectX;

Transform::Transform() {
	//set inital transform values
	SetPosition(XMFLOAT3(0, 0, 0));
	SetRotation(XMFLOAT3(0, 0, 0));
	SetScale(XMFLOAT3(1, 1, 1));
	
	//create initial matrix
	//where to store it (pointer), what to store
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&worldMatrixInverseTranspose, XMMatrixIdentity());
	needUpdate = false;
	parent = nullptr;
}

Transform::~Transform() {}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 in_pos)
{
	//using math library
	//load previous position and offset
	XMVECTOR prevPos = XMLoadFloat3(&position);
	XMVECTOR offset = XMLoadFloat3(&in_pos);
	//add and store into position
	XMStoreFloat3(&position, prevPos + offset);
	needUpdate = true;
}

void Transform::MoveRelative(DirectX::XMFLOAT3 in_pos)
{
	XMVECTOR moveVec = XMLoadFloat3(&in_pos);

	//vector rotated to match objects orientation
	XMVECTOR rotatedVec = XMVector3Rotate(
		//vector to rotate
		moveVec,
		//quaternion that represents rotation
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))
	);
	
	//add rotated vector to old position, and update
	XMStoreFloat3(&position, XMLoadFloat3(&position) + rotatedVec);

	//matrix has been changed
	needUpdate = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 in_rot)
{
	XMVECTOR prevRot = XMLoadFloat3(&rotation);
	XMVECTOR offset = XMLoadFloat3(&in_rot);
	XMStoreFloat3(&rotation, prevRot + offset);
	UpdateVectors();
	needUpdate = true;
	recalcNormals = true;
}

void Transform::Scale(DirectX::XMFLOAT3 in_scale)
{
	XMVECTOR prevScale = XMLoadFloat3(&scale);
	XMVECTOR offset = XMLoadFloat3(&in_scale);
	XMStoreFloat3(&scale, prevScale * offset);
	needUpdate = true;
}

void Transform::SetPosition(DirectX::XMFLOAT3 in_pos)
{
	position = in_pos;
	needUpdate = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 in_rot)
{
	rotation = in_rot;
	UpdateVectors();
	needUpdate = true;
	recalcNormals = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 in_scale)
{
	scale = in_scale;
	needUpdate = true;
}

void Transform::SetTransformsFromMatrix(DirectX::XMFLOAT4X4 newWorldMatrix)
{
	XMVECTOR pos;
	//Gets stored as a quaternion
	XMVECTOR rot;
	XMVECTOR l_scale;

	XMMatrixDecompose(&l_scale, &rot, &pos, XMLoadFloat4x4(&newWorldMatrix));

	//Makes sure to store both the Euler and quaternion
	XMStoreFloat4(&rotationQuat, rot);
	rotation = QuatToEuler(rotationQuat);

	XMStoreFloat3(&position, pos);
	XMStoreFloat3(&scale, l_scale);

	needUpdate = true;
	recalcNormals = true;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetRotation()
{
	return rotation;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

//return objcs right vector
DirectX::XMFLOAT3 Transform::GetRight()
{
	return right;
}

//return object's up vector
DirectX::XMFLOAT3 Transform::GetUp()
{	
	return up;
}

//get object's forward vector
DirectX::XMFLOAT3 Transform::GetForward()
{
	return forward;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	//only update world matrix when we ask for it
	//prevents extra unneccessary calculations if pos, rot, scale changes multiple times before getting world matrix
	if (needUpdate) {
		RecalcWorldAndInverseTranspose();
	}

	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrixInverseTranspose() {
	//only update world matrix when we ask for it
//prevents extra unneccessary calculations if pos, rot, scale changes multiple times before getting world matrix
	if (needUpdate) {
		RecalcWorldAndInverseTranspose();
	}

	return worldMatrixInverseTranspose;
}

//void Transform::UpdateMatrices() {
//	//create individual transform matrices for each type
//	XMMATRIX transMat = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
//	//does rotation in roll, then pitch, then yaw. Params are passed in as pitch, then yaw, then roll
//	XMMATRIX rotMat = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
//	XMMATRIX scaleMat = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
//	XMMATRIX world = scaleMat * rotMat * transMat;
//
//	// Is there a parent?
//	if (parent)
//	{
//		XMFLOAT4X4 parentWorld = parent->GetWorldMatrix();
//		world *= XMLoadFloat4x4(&parentWorld);
//	}
//	
//	XMStoreFloat4x4(&worldMatrix, world);
//	//store inverse
//	XMStoreFloat4x4(&worldMatrixInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(world)));
//
//	//matrix has been updated
//	needUpdate = false;
//}

void Transform::UpdateVectors() {
	//rotate right vector
	XMVECTOR rotatedVec = XMVector3Rotate(
		//vector to rotate
		XMVectorSet(1, 0, 0, 0),
		//quaternion that represents rotation
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))
	);

	//add rotated vector to old position, and update
	XMStoreFloat3(&right, rotatedVec);

	//rotate up vector
	rotatedVec = XMVector3Rotate(
		//vector to rotate
		XMVectorSet(0, 1, 0, 0),
		//quaternion that represents rotation
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))
	);

	//add rotated vector to old position, and update
	XMStoreFloat3(&up, rotatedVec);

	//rotate forwward vector
	rotatedVec = XMVector3Rotate(
		//vector to rotate
		XMVectorSet(0, 0, 1, 0),
		//quaternion that represents rotation
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))
	);

	//add rotated vector to old position, and update
	XMStoreFloat3(&forward, rotatedVec);
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

	children.push_back(child);
	child->parent = this;

	child->needUpdate = true;
	child->recalcNormals = true;
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

	children.erase(children.begin() + childIndex);
	child->parent = nullptr;
	child->needUpdate = true;
	child->recalcNormals = true;
	child->MarkChildrenDirty();
}

Transform* Transform::RemoveChildByIndex(unsigned int index)
{
	if (index >= children.size())
		return nullptr;

	Transform* removedChild = children[index];
	children.erase(children.begin() + index);

	removedChild->parent = nullptr;
	removedChild->needUpdate = true;
	removedChild->recalcNormals = true;
	removedChild->MarkChildrenDirty();

	return removedChild;
}

void Transform::SetParent(Transform* newParent)
{
	if (parent)
		parent->RemoveChild(this);

	//This works because the m_pParent field is set in add child
	if (newParent)
		newParent->AddChild(this);
}

void Transform::MarkChildrenDirty()
{
	for (auto& child : children) {
		child->needUpdate = true;
		child->recalcNormals = true;
	}
}

void Transform::RecalcWorldAndInverseTranspose()
{
	if (!needUpdate)
		return;

	XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX l_scale = XMMatrixScaling(scale.x, scale.y, scale.z);

	XMMATRIX worldMat = l_scale * rot * translation;

	// Is there a parent?
	if (parent)
	{
		XMFLOAT4X4 parentWorld = parent->GetWorldMatrix();
		worldMat *= XMLoadFloat4x4(&parentWorld);
	}

	XMStoreFloat4x4(&worldMatrix, worldMat);
	XMStoreFloat4x4(&worldMatrixInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));
	needUpdate = false;
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
