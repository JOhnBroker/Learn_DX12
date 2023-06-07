#include "Light.h"

using namespace DirectX;

Light::Light()
{
}

Light::~Light() 
{
}

DirectX::XMFLOAT3 Light::GetPosition() const
{
    return m_Position;
}

DirectX::XMVECTOR Light::GetPositionXM() const
{
    return XMLoadFloat3(&m_Position);
}

void Light::SetPosition(float x, float y, float z)
{
    m_Position = XMFLOAT3{ x,y,z };
    m_ViewDirty = true;
}

void Light::SetPositionXM(const DirectX::XMFLOAT3& pos)
{
    m_Position = pos;
    m_ViewDirty = true;
}

void Light::SetDirection(float x, float y, float z)
{
    m_Direction = XMFLOAT3{ x,y,z };
}

void Light::SetDirectionXM(const DirectX::XMFLOAT3& direction)
{
    m_Direction = direction;
}

void Light::SetTarget(float x, float y, float z)
{
    m_Target = XMFLOAT3(x, y, z);
}

void Light::SetTargetXM(const DirectX::XMFLOAT3& target)
{
    m_Target = target;
}

void Light::SetDistance(float dist)
{
    m_Distance = dist;
}

DirectX::XMFLOAT3 Light::GetLightDirection() const
{
    return m_Direction;
}

DirectX::XMVECTOR Light::GetLightDirectionXM() const
{
    return XMLoadFloat3(&m_Direction);
}

DirectX::XMFLOAT4X4 Light::GetView() const
{
    assert(!m_ViewDirty);
    return m_View;
}

DirectX::XMMATRIX Light::GetViewXM() const
{
    assert(!m_ViewDirty);
    return XMLoadFloat4x4(&m_View);
}

DirectX::XMFLOAT4X4 Light::GetProj() const
{
    return m_Proj;
}

DirectX::XMMATRIX Light::GetProjXM() const
{
    return XMLoadFloat4x4(&m_Proj);
}

DirectX::XMMATRIX Light::GetViewProjXM() const
{
    return GetViewXM() * GetProjXM();
}

DirectX::XMMATRIX Light::GetTransform() const
{
    return GetViewProjXM() * XMLoadFloat4x4(&m_Transform);
}

float Light::GetNearZ() const
{
    return m_NearZ;
}

float Light::GetFarZ() const
{
    return m_FarZ;
}

void Light::RotateX(float rad)
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

    XMVECTOR directionVec = XMLoadFloat3(&m_Direction);
    XMVECTOR newPosition = XMVectorMultiplyAdd(XMVectorReplicate(-m_Distance), directionVec, XMLoadFloat3(&m_Position));
    XMStoreFloat3(&m_Position, newPosition);

    m_ViewDirty = true;
}

void Light::RotateY(float rad)
{
    m_Rotation.y += rad;

    SetPositionXM(m_Target);

    XMMATRIX R = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&m_Rotation));
    XMStoreFloat3(&m_Up, XMVector3Normalize(R.r[1]));
    XMStoreFloat3(&m_Direction, XMVector3Normalize(R.r[2]));

    XMVECTOR directionVec = XMLoadFloat3(&m_Direction);
    XMVECTOR newPosition = XMVectorMultiplyAdd(XMVectorReplicate(-m_Distance), directionVec, XMLoadFloat3(&m_Position));
    XMStoreFloat3(&m_Position, newPosition);

    m_ViewDirty = true;
}

void Light::UpdateViewMatrix()
{
    if (m_ViewDirty) 
    {
        XMVECTOR lightPos = XMLoadFloat3(&m_Position);
        XMVECTOR targetPos = XMLoadFloat3(&m_Target);
        XMVECTOR lightUp = XMLoadFloat3(&m_Up);
        XMMATRIX view = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

        XMStoreFloat4x4(&m_View, view);

        m_ViewDirty = false;
    }
}

DirectionalLight::DirectionalLight()
    :Light()
{
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::SetFrustum(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    m_NearZ = nearZ;
    m_FarZ = farZ;

    XMMATRIX P = XMMatrixOrthographicOffCenterLH(left, right, bottom, top, m_NearZ, m_FarZ);
    XMStoreFloat4x4(&m_Proj, P);
}

SpotLight::SpotLight()
    :Light()
{
}

SpotLight::~SpotLight()
{
}

void SpotLight::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
    m_NearZ = nearZ;
    m_FarZ = farZ;

    XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, m_NearZ, m_FarZ);
    XMStoreFloat4x4(&m_Proj, P);
}
