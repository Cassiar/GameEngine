#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(DirectX::XMFLOAT3 pos, float in_aspectRatio, float in_fov, float in_nearPlane, float in_farPlane)
{
    //set initial position
    transform.SetPosition(pos);
    fov = in_fov;
    aspectRatio = in_aspectRatio;
    nearPlane = in_nearPlane;
    farPlane = in_farPlane;
    UpdateViewMatrix();
    UpdateProjectionMatrix(aspectRatio);

    moveSpeed = 10.0f;
    mouseMoveSpeed = 1.0f;
}

Camera::~Camera() {}

void Camera::Update(float dt)
{
    //get reference to input manager
    Input& input = Input::GetInstance();
    
    float speed = moveSpeed * dt;

    //adjust speed on modifiers
    if (input.KeyDown(VK_SHIFT)) {
        speed *= 2;
    }
    if (input.KeyDown(VK_LCONTROL)) {
        speed *= 0.5f;
    }

    //check for keyboard input
    //move forward/back
    if (input.KeyDown('W')) {
        transform.MoveRelative(XMFLOAT3(0, 0, speed));
    }
    else if (input.KeyDown('S')) {
        transform.MoveRelative(XMFLOAT3(0, 0, -speed));
    }

    //strafe
    if (input.KeyDown('A')) {
        transform.MoveRelative(XMFLOAT3(-speed, 0, 0));
    }    
    else if (input.KeyDown('D')) {
        transform.MoveRelative(XMFLOAT3(speed, 0, 0));
    }

    //move up/down, vertically
    if (input.KeyDown(' ')) {
        transform.MoveAbsolute(XMFLOAT3(0, speed, 0));
    }
    else if (input.KeyDown('X')) {
        transform.MoveAbsolute(XMFLOAT3(0, -speed, 0));
    }

    //rotate camera
    if (input.MouseLeftDown()) {
        float mouseMoveX = input.GetMouseXDelta() * mouseMoveSpeed * dt;
        float mouseMoveY = input.GetMouseYDelta() * mouseMoveSpeed * dt;

        //rotaion goes in the opposite order. moving mouse left and right (x dir) moves camera in y
        transform.Rotate(XMFLOAT3(mouseMoveY, mouseMoveX, 0));

        XMFLOAT3 rotations = transform.GetRotation();
        //lock movement so doesn't start going upside down
        if (rotations.x < -0.95f * XM_PIDIV2) {
            transform.SetRotation(XMFLOAT3(-0.95f * XM_PIDIV2, rotations.y, rotations.z));
        }
        else if (rotations.x > 0.95f * XM_PIDIV2) {
            transform.SetRotation(XMFLOAT3(0.95f * XM_PIDIV2, rotations.y, rotations.z));
        }
    }

    UpdateViewMatrix();

    //change fov
    if (input.KeyDown(VK_OEM_PLUS)) {
        fov += dt;
    }
    else if (input.KeyDown(VK_OEM_MINUS)) {
        fov -= dt;
    }

    UpdateProjectionMatrix(aspectRatio);
}

void Camera::UpdateViewMatrix()
{
    //get forward vector, will get rolled into transform class
   
    XMFLOAT3 rot = transform.GetRotation();
    XMFLOAT3 pos = transform.GetPosition();
    XMFLOAT3 front = transform.GetForward();

    //rotate the x-axis to match the orientation of the camera
    XMVECTOR forward = XMLoadFloat3(&front);
    XMMATRIX view = XMMatrixLookToLH(
        //where the camera is 
        XMLoadFloat3(&pos),
        //the camera's forward vector
        forward,
        //world's up axis
        XMVectorSet(0, 1, 0, 0)
    );

    //view = XMMatrixLookAtLH(
    //    XMLoadFloat3(&pos),
    //    XMVectorSet(0, 0, 0, 0),
    //    XMVectorSet(0, 1, 0, 0)
    //);

    XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        fov, //fov, how wide in radians, pi/2 = 90 degrees
        aspectRatio, //aspect ratio of box, match windows
        nearPlane, //near plane, can't be zero
        farPlane //far plane, usually never over 1000
    );

    XMStoreFloat4x4(&projMatrix, proj);
}

Transform* Camera::GetTransform()
{
    return &transform;
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
    return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
    return projMatrix;
}
