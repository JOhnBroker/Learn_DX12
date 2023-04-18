#ifndef CAMERA_H
#define CAMERA_H

#include "d3dUtil.h"

class Camera
{
public:
	Camera();
	~Camera();

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMVECTOR GetPositionXM() const;
	void SetPosition(float x, float y, float z);
	void SetPositionXM(const DirectX::XMFLOAT3& pos);

	DirectX::XMFLOAT3 GetRightAxis()const;
	DirectX::XMVECTOR GetRightAxisXM()const;
	DirectX::XMFLOAT3 GetUpAxis()const;
	DirectX::XMVECTOR GetUpAxisXM()const;
	DirectX::XMFLOAT3 GetLookAxis()const;
	DirectX::XMVECTOR GetLookAxisXM()const;

	DirectX::XMFLOAT4X4 GetView()const;
	DirectX::XMMATRIX GetViewXM()const;
	DirectX::XMFLOAT4X4 GetProj()const;
	DirectX::XMMATRIX GetProjXM()const;
	DirectX::XMMATRIX GetViewProjXM()const;

	void SetFrustum(float fovY, float aspect, float nearZ, float farZ);
	void SetViewPort(const D3D12_VIEWPORT& viewPort);
	void SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
	D3D12_VIEWPORT GetViewPort() const;

	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp);

	void Strafe(float d);
	void Walk(float d);
	// 上下观察
	// 正rad值向上观察
	// 负rad值向下观察
	void Pitch(float angle);
	// 左右观察
	// 正rad值向右观察
	// 负rad值向左观察
	void RotateY(float angle);

	void UpdateViewMatrix();

protected:

	DirectX::XMFLOAT3 m_Position = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 m_Right = { 1.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 m_Up = { 0.0f,1.0f,0.0f };
	DirectX::XMFLOAT3 m_Look = { 0.0f,0.0f,1.0f };

	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Aspect = 0.0f;
	float m_FovY = 0.0f;

	bool m_ViewDirty = true;

	D3D12_VIEWPORT  m_ViewPort = {};

	DirectX::XMFLOAT4X4 m_View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();

};
#endif // !CAMERA_H
