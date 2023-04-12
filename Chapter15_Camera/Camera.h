#ifndef CAMERA_H
#define CAMERA_H

#include "Transform.h"

class Camera
{
public:
	Camera() = default;
	~Camera() = default;

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMVECTOR GetPositionXM() const;

	float GetRotationX()const;
	float GetRotationY()const;

	DirectX::XMFLOAT3 GetRightAxis()const;
	DirectX::XMVECTOR GetRightAxisXM()const;
	DirectX::XMFLOAT3 GetUpAxis()const;
	DirectX::XMVECTOR GetUpAxisXM()const;
	DirectX::XMFLOAT3 GetLookAxis()const;
	DirectX::XMVECTOR GetLookAxisXM()const;

	DirectX::XMMATRIX GetViewXM()const;
	DirectX::XMMATRIX GetProjXM()const;
	DirectX::XMMATRIX GetViewProjXM()const;

	D3D12_VIEWPORT GetViewPort() const;

	void SetFrustum(float fovY, float aspect, float nearZ, float farZ);
	void SetViewPort(const D3D12_VIEWPORT& viewPort);
	void SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

protected:
	Transform m_Transfor = {};
	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Aspect = 0.0f;
	float m_FovY = 0.0f;

	D3D12_VIEWPORT  m_ViewPort = {};
};


#endif // !CAMERA_H
