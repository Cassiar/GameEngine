//Author: Cassiar Beaver
//Represents a posistion in 3d space using a matrix4x4
//most game entities will use this
#pragma once

#include <DirectXMath.h>
#include <vector>

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
	void SetTransformsFromMatrix(DirectX::XMFLOAT4X4 newWorldMatrix);

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

	void AddChild(Transform* child, bool makeRelative = true);
	void RemoveChild(Transform* child);
	Transform* RemoveChildByIndex(unsigned int index);
	void SetParent(Transform* newParent);
	void MarkChildrenDirty();

	Transform* GetParent() const { return parent; }
	Transform* GetChild(unsigned int index) const { return (index < children.size()) ? children[index] : nullptr; }
	int GetNumChildren() const { return static_cast<int>(children.size()); }
	int GetChildIndex(Transform* child) const {
		if (!child) return -1;
		for (unsigned int i = 0; i < children.size(); i++)
			if (children[i] == child)
				return i;
		return -1;
	}

private:
	//Recalcutaes the m_m4WorldMatrix and m_m4WorldInverseTranspose member variables
	//based on the values of the position, scale, and rotation the Transform is 
	//currently in.
	void RecalcWorldAndInverseTranspose();
	void UpdateVectors();

	Transform* parent;
	std::vector<Transform*> children;

	//raw transform data: pos, rot, scale
	DirectX::XMFLOAT3 position;
	//pitch (x), yaw (y), roll (z)
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 scale;

	DirectX::XMFLOAT4 rotationQuat;

	//right, up, and forward vectors
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;

	//finalized matrix. combination of above
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldMatrixInverseTranspose;

	//does our matrix need an update
	bool needUpdate;
	bool recalcNormals;

	DirectX::XMFLOAT3 QuatToEuler(DirectX::XMFLOAT4 quat);
};

