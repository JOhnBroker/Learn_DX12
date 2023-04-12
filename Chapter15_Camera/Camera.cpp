#include "Camera.h"

DirectX::XMFLOAT3 Camera::GetPosition() const
{
    return DirectX::XMFLOAT3();
}

DirectX::XMVECTOR Camera::GetPositionXM() const
{
    return DirectX::XMVECTOR();
}

float Camera::GetRotationX() const
{
    return 0.0f;
}

float Camera::GetRotationY() const
{
    return 0.0f;
}

DirectX::XMFLOAT3 Camera::GetRightAxis() const
{
    return DirectX::XMFLOAT3();
}

DirectX::XMVECTOR Camera::GetRightAxisXM() const
{
    return DirectX::XMVECTOR();
}

DirectX::XMFLOAT3 Camera::GetUpAxis() const
{
    return DirectX::XMFLOAT3();
}

DirectX::XMVECTOR Camera::GetUpAxisXM() const
{
    return DirectX::XMVECTOR();
}

DirectX::XMFLOAT3 Camera::GetLookAxis() const
{
    return DirectX::XMFLOAT3();
}

DirectX::XMVECTOR Camera::GetLookAxisXM() const
{
    return DirectX::XMVECTOR();
}

DirectX::XMMATRIX Camera::GetViewXM() const
{
    return DirectX::XMMATRIX();
}

DirectX::XMMATRIX Camera::GetProjXM() const
{
    return DirectX::XMMATRIX();
}

DirectX::XMMATRIX Camera::GetViewProjXM() const
{
    return DirectX::XMMATRIX();
}

D3D12_VIEWPORT Camera::GetViewPort() const
{
    return D3D12_VIEWPORT();
}

void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
}

void Camera::SetViewPort(const D3D12_VIEWPORT& viewPort)
{
}

void Camera::SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
{
}
