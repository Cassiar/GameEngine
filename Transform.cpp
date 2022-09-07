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
}

void Transform::SetScale(DirectX::XMFLOAT3 in_scale)
{
	scale = in_scale;
	needUpdate = true;
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
		UpdateMatrices();
	}

	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrixInverseTranspose() {
	//only update world matrix when we ask for it
//prevents extra unneccessary calculations if pos, rot, scale changes multiple times before getting world matrix
	if (needUpdate) {
		UpdateMatrices();
	}

	return worldMatrixInverseTranspose;
}

void Transform::UpdateMatrices() {
	//create individual transform matrices for each type
	XMMATRIX transMat = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	//does rotation in roll, then pitch, then yaw. Params are passed in as pitch, then yaw, then roll
	XMMATRIX rotMat = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
	XMMATRIX scaleMat = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
	XMMATRIX world = scaleMat * rotMat * transMat;
	
	XMStoreFloat4x4(&worldMatrix, world);
	//store inverse
	XMStoreFloat4x4(&worldMatrixInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(world)));

	//matrix has been updated
	needUpdate = false;
}

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