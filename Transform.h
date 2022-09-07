//Author: Cassiar Beaver
//Represents a posistion in 3d space using a matrix4x4
//most game entities will use this
#pragma once

#include <DirectXMath.h>

class Transform
{
public:
	//assume data starts at defualt
	Transform();
	~Transform();

	//move within the world axis. regardless of obj rotation
	void MoveAbsolute(DirectX::XMFLOAT3 in_pos);
	//move relative to the object's rotation
	void MoveRelative(DirectX::XMFLOAT3 in_pos);
	void Rotate(DirectX::XMFLOAT3 in_rot);
	void Scale(DirectX::XMFLOAT3 in_scale);

	//override the the pos, rot, or scale
	void SetPosition(DirectX::XMFLOAT3 in_pos);
	void SetRotation(DirectX::XMFLOAT3 in_rot);
	void SetScale(DirectX::XMFLOAT3 in_scale);

	//get the pos, rot, scale, 
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotation();
	DirectX::XMFLOAT3 GetScale();
	//get right, up, or forward vectors
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetForward();
	//get world matrix or inverse
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldMatrixInverseTranspose();

private:
	void UpdateMatrices();
	void UpdateVectors();
	//raw transform data: pos, rot, scale
	DirectX::XMFLOAT3 position;
	//pitch (x), yaw (y), roll (z)
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 scale;

	//right, up, and forward vectors
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;

	//finalized matrix. combination of above
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldMatrixInverseTranspose;

	//does our matrix need an update
	bool needUpdate;
};

