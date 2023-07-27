#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
    SetFrustum(0.25f * MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
}

Camera::~Camera()
{
}

DirectX::XMFLOAT3 Camera::GetPosition() const
{
    return m_Transform.GetPosition();
}

DirectX::XMVECTOR Camera::GetPositionXM() const
{
    return m_Transform.GetPositionXM();
}

float Camera::GetRotationX() const
{
    return m_Transform.GetRotation().x;
}

float Camera::GetRotationY() const
{
    return m_Transform.GetRotation().y;
}

DirectX::XMFLOAT3 Camera::GetRightAxis() const
{
    return m_Transform.GetRightAxis();
}

DirectX::XMVECTOR Camera::GetRightAxisXM() const
{
    return m_Transform.GetRightAxisXM();
}

DirectX::XMFLOAT3 Camera::GetUpAxis() const
{
    return m_Transform.GetUpAxis();
}

DirectX::XMVECTOR Camera::GetUpAxisXM() const
{
    return m_Transform.GetUpAxisXM();
}

DirectX::XMFLOAT3 Camera::GetLookAxis() const
{
    return m_Transform.GetForwardAxis();
}

DirectX::XMVECTOR Camera::GetLookAxisXM() const
{
    return m_Transform.GetForwardAxisXM();
}

DirectX::XMMATRIX Camera::GetLocalToWorldMatrixXM() const
{
    return m_Transform.GetLocalToWorldMatrixXM();
}

DirectX::XMMATRIX Camera::GetViewMatrixXM() const
{
    return m_Transform.GetWorldToLocalMatrixXM();
}

DirectX::XMMATRIX Camera::GetProjMatrixXM(bool reversedZ) const
{
    if (reversedZ) 
    {
        return XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_FarZ, m_NearZ);
    }
    else 
    {
        return XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearZ, m_FarZ);
    }
}

DirectX::XMMATRIX Camera::GetViewProjMatrixXM(bool reversedZ) const
{
    return GetViewMatrixXM()* GetProjMatrixXM();
}

float Camera::GetNearZ() const
{
    return m_NearZ;
}

float Camera::GetFarZ() const
{
    return m_FarZ;
}

float Camera::GetFovY() const
{
    return m_FovY;
}

float Camera::GetAspectRatio() const
{
    return m_Aspect;
}

void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
    m_FovY = fovY;
    m_Aspect = aspect;
    m_NearZ = nearZ;
    m_FarZ = farZ;
}

void Camera::SetViewPort(const D3D12_VIEWPORT& viewPort)
{
    m_ViewPort = viewPort;
}

void Camera::SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
{
    m_ViewPort.TopLeftX = topLeftX;
    m_ViewPort.TopLeftY = topLeftY;
    m_ViewPort.Width = width;
    m_ViewPort.Height = height;
    m_ViewPort.MinDepth = minDepth;
    m_ViewPort.MaxDepth = maxDepth;
}

FirstPersonCamera::FirstPersonCamera() 
    :Camera()
{
}

FirstPersonCamera::~FirstPersonCamera()
{
}

void FirstPersonCamera::SetPosition(float x, float y, float z)
{
    m_Transform.SetPosition(XMFLOAT3{ x,y,z });
}

void FirstPersonCamera::SetPositionXM(const DirectX::XMFLOAT3& pos)
{
    m_Transform.SetPosition(pos);
}

void FirstPersonCamera::LookAt(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target, DirectX::XMFLOAT3 up)
{
    m_Transform.SetPosition(pos);
    m_Transform.LookAt(target, up);
}

void FirstPersonCamera::LookTo(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT3& up)
{
    m_Transform.SetPosition(pos);
    m_Transform.LookTo(to, up);
}

void FirstPersonCamera::Strafe(float d)
{
	// 左右移动
    m_Transform.Translate(m_Transform.GetRightAxis(), d);
}

void FirstPersonCamera::Walk(float d)
{
    XMVECTOR rightVec = m_Transform.GetRightAxisXM();
    XMVECTOR frontVec = XMVector3Normalize(XMVector3Cross(rightVec, g_XMIdentityR1));
    XMFLOAT3 front;
    XMStoreFloat3(&front, frontVec);
    m_Transform.Translate(front, d);
}

void FirstPersonCamera::MoveForward(float d)
{
	// 前后移动
    m_Transform.Translate(m_Transform.GetForwardAxis(), d);
}

void FirstPersonCamera::Translate(const DirectX::XMFLOAT3& dir, float magnitude)
{
    m_Transform.Translate(dir, magnitude);
}

void FirstPersonCamera::Pitch(float rad)
{
    XMFLOAT3 rotation = m_Transform.GetRotation();

    rotation.x += rad;
    if (rotation.x > XM_PI * 7 / 18)
        rotation.x = XM_PI * 7 / 18;
    else if (rotation.x < -XM_PI * 7 / 18)
        rotation.x = -XM_PI * 7 / 18;
    m_Transform.SetRotation(rotation);
}

void FirstPersonCamera::RotateY(float rad)
{
    XMFLOAT3 rotation = m_Transform.GetRotation();
    rotation.y = XMScalarModAngle(rotation.y + rad);
    m_Transform.SetRotation(rotation);
}

ThirdPersonCamera::ThirdPersonCamera() 
    :Camera()
{
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

DirectX::XMFLOAT3 ThirdPersonCamera::GetTargetPosition() const
{
    return m_Target;
}

float ThirdPersonCamera::GetDistance() const
{
    return m_Distance;
}

void ThirdPersonCamera::RotateX(float rad)
{
    XMFLOAT3 rotation = m_Transform.GetRotation();
    rotation.x += rad;
    if (rotation.x < 0.0f)
    {
        rotation.x = 0.0f;
    }
    else if (rotation.x > XM_PI / 3)
    {
        rotation.x = XM_PI / 3;
    }
    m_Transform.SetRotation(rotation);
    m_Transform.SetPosition(m_Target);
    m_Transform.Translate(m_Transform.GetForwardAxis(), -m_Distance);
}

void ThirdPersonCamera::RotateY(float rad)
{
    XMFLOAT3 rotation = m_Transform.GetRotation();
    rotation.y = XMScalarModAngle(rotation.y + rad);

    m_Transform.SetRotation(rotation);
    m_Transform.SetPosition(m_Target);
    m_Transform.Translate(m_Transform.GetForwardAxis(), -m_Distance);
}

void ThirdPersonCamera::Approach(float dist)
{
    m_Distance += dist;
    if (m_Distance < m_MinDist) 
    {
        m_Distance = m_MinDist;
    }
    else if (m_Distance > m_MaxDist) 
    {
        m_Distance = m_MaxDist;
    }
    
    m_Transform.SetPosition(m_Target);
    m_Transform.Translate(m_Transform.GetForwardAxis(), -m_Distance);
}

void ThirdPersonCamera::SetRotationX(float rad)
{
    XMFLOAT3 rotation = m_Transform.GetRotation();
    rotation.x = rad;
    if (rotation.x < 0.0f)
    {
        rotation.x = 0.0f;
    }
    else if (rotation.x > XM_PI / 3)
    {
        rotation.x = XM_PI / 3;
    }
    m_Transform.SetRotation(rotation);
    m_Transform.SetPosition(m_Target);
    m_Transform.Translate(m_Transform.GetForwardAxis(), -m_Distance);
}

void ThirdPersonCamera::SetRotationY(float rad)
{
    XMFLOAT3 rotation = m_Transform.GetRotation();
    rotation.y = XMScalarModAngle(rad);

    m_Transform.SetRotation(rotation);
    m_Transform.SetPosition(m_Target);
    m_Transform.Translate(m_Transform.GetForwardAxis(), -m_Distance);
}

void ThirdPersonCamera::SetTarget(const DirectX::XMFLOAT3& target)
{
    m_Target = target;
}

void ThirdPersonCamera::SetDistance(float dist)
{
    m_Distance = dist;
}

void ThirdPersonCamera::SetDistanceMinMax(float minDist, float maxDist)
{
    m_MinDist = minDist;
    m_MaxDist = maxDist;
}
