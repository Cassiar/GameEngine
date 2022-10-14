#pragma once

#include <DirectXMath.h>
#include <vector>

class Transform
{
private:
	Transform* m_pParent;
	std::vector<Transform*> m_lChildren;

	DirectX::XMFLOAT3 m_v3Position;
	DirectX::XMFLOAT3 m_v3EulerAngles;
	DirectX::XMFLOAT3 m_v3Scale;

	DirectX::XMFLOAT4 m_v4RotationQuat;

	//Transform directions
	DirectX::XMFLOAT3 m_v3Forward;
	DirectX::XMFLOAT3 m_v3Up;
	DirectX::XMFLOAT3 m_v3Right;

	DirectX::XMFLOAT4X4 m_m4WorldMatrix;
	DirectX::XMFLOAT4X4 m_m4WorldInverseTranspose;

	//Does matrix need an update
	bool m_bRecalcWorld;
	//Do the normals need an update;
	bool m_bRecalcNormals;

	//Recalcutaes the m_m4WorldMatrix and m_m4WorldInverseTranspose member variables
	//based on the values of the position, scale, and rotation the Transform is 
	//currently in.
	void RecalcWorldAndInverseTranspose();

	//Recalculates the Forward, Right, and Up vectors
	void RecalcNormals();

	DirectX::XMFLOAT3 QuatToEuler(DirectX::XMFLOAT4 quat);

public:
	Transform();

	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 absoluteMoveVec) { this->MoveAbsolute(absoluteMoveVec.x, absoluteMoveVec.y, absoluteMoveVec.z); };

	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 relativeMoveVec) { this->MoveRelative(relativeMoveVec.x, relativeMoveVec.y, relativeMoveVec.z); };

	void Rotate(float p, float y, float r);
	void Rotate(DirectX::XMFLOAT3 rotateVec) { this->Rotate(rotateVec.x, rotateVec.y, rotateVec.z); };

	void Scale(float x, float y, float z);
	void Scale(float scalar);

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 newPos);
	void SetRotation(float p, float y, float r);
	void SetRotation(DirectX::XMFLOAT3 newRot);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 newScale);
	void SetTransformsFromMatrix(DirectX::XMFLOAT4X4 newWorldMatrix);

	DirectX::XMFLOAT3 GetPosition() const { return m_v3Position; }
	DirectX::XMFLOAT3 GetEulerAngles() const { return m_v3EulerAngles; }
	DirectX::XMFLOAT3 GetRotation() const { return m_v3EulerAngles; }
	DirectX::XMFLOAT3 GetScale() const { return m_v3Scale; }

	DirectX::XMFLOAT4 GetRotationQuat() const { return m_v4RotationQuat; }

	DirectX::XMFLOAT3 GetForward();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetRight();

	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

	void AddChild(Transform* child, bool makeRelative = true);
	void RemoveChild(Transform* child);
	Transform* RemoveChildByIndex(unsigned int index);
	void SetParent(Transform* newParent);
	void MarkChildrenDirty();

	Transform* GetParent() const { return m_pParent; }
	Transform* GetChild(unsigned int index) const { return (index < m_lChildren.size()) ? m_lChildren[index] : nullptr; }
	int GetNumChildren() const { return static_cast<int>(m_lChildren.size()); }
	int GetChildIndex(Transform* child) const {
		if (!child) return -1;
		for (unsigned int i = 0; i < m_lChildren.size(); i++)
			if (m_lChildren[i] == child)
				return i;
		return -1;
	}

	bool IsWorldDirty() { return m_bRecalcWorld; }

};

