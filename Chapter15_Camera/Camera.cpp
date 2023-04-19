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
    return m_Position;
}

DirectX::XMVECTOR Camera::GetPositionXM() const
{
    return XMLoadFloat3(&m_Position);
}

void Camera::SetPosition(float x, float y, float z)
{
    m_Position = XMFLOAT3(x, y, z);
    m_ViewDirty = true;
}

void Camera::SetPositionXM(const DirectX::XMFLOAT3& pos)
{
    m_Position = pos;
    m_ViewDirty = true;
}

DirectX::XMFLOAT3 Camera::GetRightAxis() const
{
    return m_Right;
}

DirectX::XMVECTOR Camera::GetRightAxisXM() const
{
    return XMLoadFloat3(&m_Right);
}

DirectX::XMFLOAT3 Camera::GetUpAxis() const
{
    return m_Up;
}

DirectX::XMVECTOR Camera::GetUpAxisXM() const
{
    return XMLoadFloat3(&m_Up);
}

DirectX::XMFLOAT3 Camera::GetLookAxis() const
{
    return m_Look;
}

DirectX::XMVECTOR Camera::GetLookAxisXM() const
{
    return XMLoadFloat3(&m_Look);
}

DirectX::XMFLOAT4X4 Camera::GetView() const
{
    assert(!m_ViewDirty);
    return m_View;
}

DirectX::XMMATRIX Camera::GetViewXM() const
{
    assert(!m_ViewDirty);
    return XMLoadFloat4x4(&m_View);
}

DirectX::XMFLOAT4X4 Camera::GetProj() const
{
    return m_Proj;
}

DirectX::XMMATRIX Camera::GetProjXM() const
{
    return XMLoadFloat4x4(&m_Proj);
}

DirectX::XMMATRIX Camera::GetViewProjXM() const
{
    return GetViewXM() * GetProjXM();
}

D3D12_VIEWPORT Camera::GetViewPort() const
{
    return m_ViewPort;
}

void Camera::UpdateViewMatrix()
{
    if (m_ViewDirty) 
    {
        XMVECTOR R = XMLoadFloat3(&m_Right);
        XMVECTOR U = XMLoadFloat3(&m_Up);
        XMVECTOR L = XMLoadFloat3(&m_Look);
        XMVECTOR P = XMLoadFloat3(&m_Position);

        L = XMVector3Normalize(L);
        U = XMVector3Normalize(XMVector3Cross(L, R));

        R = XMVector3Cross(U, L);

        float x = -XMVectorGetX(XMVector3Dot(P, R));
        float y = -XMVectorGetX(XMVector3Dot(P, U));
        float z = -XMVectorGetX(XMVector3Dot(P, L));

        XMStoreFloat3(&m_Right, R);
        XMStoreFloat3(&m_Up, U);
        XMStoreFloat3(&m_Look, L);

        m_View(0, 0) = m_Right.x;
        m_View(1, 0) = m_Right.y;
        m_View(2, 0) = m_Right.z;
        m_View(3, 0) = x;

        m_View(0, 1) = m_Up.x;
        m_View(1, 1) = m_Up.y;
        m_View(2, 1) = m_Up.z;
        m_View(3, 1) = y;

        m_View(0, 2) = m_Look.x;
        m_View(1, 2) = m_Look.y;
        m_View(2, 2) = m_Look.z;
        m_View(3, 2) = z;

        m_View(0, 3) = 0.0f;
        m_View(1, 3) = 0.0f;
        m_View(2, 3) = 0.0f;
        m_View(3, 3) = 1.0f;

        m_ViewDirty = false;
    }
}

void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
    m_FovY = fovY;
    m_Aspect = aspect;
    m_NearZ = nearZ;
    m_FarZ = farZ;

    XMMATRIX P = XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearZ, m_FarZ);
    XMStoreFloat4x4(&m_Proj, P);
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

void FirstPersonCamera::LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_Position, pos);
	XMStoreFloat3(&m_Look, L);
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);

	m_ViewDirty = true;
}

void FirstPersonCamera::LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&worldUp);

	LookAt(P, T, U);

	m_ViewDirty = true;
}

void FirstPersonCamera::Strafe(float d)
{
	// 左右移动
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&m_Right);
	XMVECTOR p = XMLoadFloat3(&m_Position);

	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, r, p));
	m_ViewDirty = true;
}

void FirstPersonCamera::Walk(float d)
{
	// 前后移动
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&m_Look);
	XMVECTOR p = XMLoadFloat3(&m_Position);

	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));
	m_ViewDirty = true;
}

void FirstPersonCamera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), angle);

	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	m_ViewDirty = true;
}

void FirstPersonCamera::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	m_ViewDirty = true;
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
    m_Rotation.x += rad;
    if (m_Rotation.x < 0.0f) 
    {
        m_Rotation.x = 0.0f;
    }
    else if (m_Rotation.x > XM_PI / 3) 
    {
        m_Rotation.x = XM_PI / 3;
    }

    SetPositionXM(m_Target);

    XMMATRIX R = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&m_Rotation));
    XMStoreFloat3(&m_Right, XMVector3Normalize(R.r[0]));
    XMStoreFloat3(&m_Up, XMVector3Normalize(R.r[1]));
    XMStoreFloat3(&m_Look, XMVector3Normalize(R.r[2]));
    
    XMVECTOR directionVec = XMLoadFloat3(&m_Look);
    XMVECTOR newPosition = XMVectorMultiplyAdd(XMVectorReplicate(-m_Distance), directionVec, XMLoadFloat3(&m_Position));
    XMStoreFloat3(&m_Position, newPosition);

	m_ViewDirty = true;
}

void ThirdPersonCamera::RotateY(float rad)
{
    m_Rotation.y += rad;

    SetPositionXM(m_Target);

    XMMATRIX R = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&m_Rotation));
    XMStoreFloat3(&m_Right, XMVector3Normalize(R.r[0]));
    XMStoreFloat3(&m_Up, XMVector3Normalize(R.r[1]));
    XMStoreFloat3(&m_Look, XMVector3Normalize(R.r[2]));

    XMVECTOR directionVec = XMLoadFloat3(&m_Look);
    XMVECTOR newPosition = XMVectorMultiplyAdd(XMVectorReplicate(-m_Distance), directionVec, XMLoadFloat3(&m_Position));
    XMStoreFloat3(&m_Position, newPosition);

    m_ViewDirty = true;
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
    SetPositionXM(m_Target);

    XMVECTOR directionVec = XMLoadFloat3(&m_Look);
    XMVECTOR newPosition = XMVectorMultiplyAdd(XMVectorReplicate(-m_Distance), directionVec, XMLoadFloat3(&m_Position));
    XMStoreFloat3(&m_Position, newPosition);

	m_ViewDirty = true;
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
