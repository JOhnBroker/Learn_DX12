#ifndef LIGHT_H
#define LIGHT_H

#include "d3dUtil.h"

#define MAXLIGHTNUM 16

struct LightConstants
{
	DirectX::XMFLOAT3 m_Strength = { 0.5f,0.5f,0.5f };
	float m_FalloffStart = 1.0f;
	DirectX::XMFLOAT3 m_Direction = { 0.0f,-1.0f,0.0f };
	float m_FalloffEnd = 10.0f;
	DirectX::XMFLOAT3 m_Position = { 0.0f,0.0f,0.0f };
	float m_SpotPower = 64.0f;
};

class Light
{
public:
	Light();
	virtual ~Light() = 0;

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMVECTOR GetPositionXM() const;
	void SetPosition(float x, float y, float z);
	void SetPositionXM(const DirectX::XMFLOAT3& pos);
	void SetDirection(float x, float y, float z);
	void SetDirectionXM(const DirectX::XMFLOAT3& direction);
	void SetTarget(float x, float y, float z);
	void SetTargetXM(const DirectX::XMFLOAT3& target);
	void SetDistance(float dist);

	DirectX::XMFLOAT3 GetLightDirection()const;
	DirectX::XMVECTOR GetLightDirectionXM()const;

	DirectX::XMFLOAT4X4 GetView()const;
	DirectX::XMMATRIX GetViewXM()const;
	DirectX::XMFLOAT4X4 GetProj()const;
	DirectX::XMMATRIX GetProjXM()const;
	DirectX::XMMATRIX GetViewProjXM()const;
	DirectX::XMMATRIX GetTransform()const;
	float GetNearZ()const;
	float GetFarZ()const;

	void RotateX(float rad);
	void RotateY(float rad);
	void UpdateViewMatrix();

protected:
	DirectX::XMFLOAT3 m_Direction = { 0.0f,-1.0f,0.0f };
	DirectX::XMFLOAT3 m_Position = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 m_Target = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 m_Up = { 0.0f,1.0f,0.0f };
	DirectX::XMFLOAT3 m_Rotation = {};	// 旋转欧拉角(弧度制)

	DirectX::XMFLOAT4X4 m_View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_Transform = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f };
	
	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Distance = 0.0f;

	bool m_ViewDirty = true;
};

class DirectionalLight :public Light 
{
	// 本质上是以正交投影的方式，投影阴影贴图至场景
public:
	DirectionalLight();
	~DirectionalLight() override;
	void SetFrustum(float left, float right, float bottom, float top, float nearZ, float farZ);	
};

class SpotLight :public Light 
{
	// 本质上是以透视投影的方式，投影阴影贴图至场景
public:
	SpotLight();
	~SpotLight() override;
	void SetFrustum(float fovY, float aspect, float nearZ, float farZ);
};

#endif
