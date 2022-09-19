//Author: Cassiar Beaver
//Represents a camera in 3D space. 
//Camera can move in all 3 directions, and can rotate in x and y (pitch and yaw).
//Camera cannot roll (rotate in z)
#pragma once

#include <DirectXMath.h>

#include "Transform.h"

class Camera
{
public:
	/// <summary>
	/// Create the camera object
	/// </summary>
	/// <param name="pos">Where camera starts in world space</param>
	/// <param name="aspectRation">The aspect ratio</param>
	Camera(DirectX::XMFLOAT3 pos, float in_aspectRatio, float in_fov, float in_nearPlane, float in_farPlane);
	~Camera();
	
	//update methods
	/// <summary>
	/// Update the position, and rotation of the camera
	/// </summary>
	/// <param name="dt">delta time</param>
	void Update(float dt);

	/// <summary>
	/// Update just view matrix
	/// </summary>
	void UpdateViewMatrix();

	/// <summary>
	/// Update the projection of the camera
	/// </summary>
	/// <param name="aspectRatio">The new aspect ration</param>
	void UpdateProjectionMatrix(float aspectRatio);

	/// <summary>
	/// Getter for camera's transform
	/// </summary>
	/// <returns>Pointer to camera's transform. Any changes will auto affect camera</returns>
	Transform* GetTransform();

	/// <summary>
	/// Get the view matrix
	/// </summary>
	/// <returns>A copy of the matrix</returns>
	DirectX::XMFLOAT4X4 GetViewMatrix();

	/// <summary>
	/// Get the projection matrix
	/// </summary>
	/// <returns>A copy of the matrix</returns>
	DirectX::XMFLOAT4X4 GetProjectionMatrix();

	float* GetMoveSpeed() { return &moveSpeed; }
	float* GetMouseMoveSpeed() { return &mouseMoveSpeed; }

private:
	//camera matrices
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;

	//overall transformation
	Transform transform;

	//field of view, in radians
	float fov;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	float moveSpeed;
	float mouseMoveSpeed;
};

