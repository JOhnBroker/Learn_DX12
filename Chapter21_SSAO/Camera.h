#ifndef CAMERA_H
#define CAMERA_H

#include "d3dUtil.h"
#include "Transform.h"

class Camera
{
public:
	Camera();
	virtual ~Camera() = 0;

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMVECTOR GetPositionXM() const;

	// 获取绕X轴旋转的欧拉角弧度
	float GetRotationX() const;
	// 获取绕Y轴旋转的欧拉角弧度
	float GetRotationY() const;

	DirectX::XMFLOAT3 GetRightAxis()const;
	DirectX::XMVECTOR GetRightAxisXM()const;
	DirectX::XMFLOAT3 GetUpAxis()const;
	DirectX::XMVECTOR GetUpAxisXM()const;
	DirectX::XMFLOAT3 GetLookAxis()const;
	DirectX::XMVECTOR GetLookAxisXM()const;

	DirectX::XMMATRIX GetLocalToWorldMatrixXM() const;
	DirectX::XMMATRIX GetViewMatrixXM() const;
	DirectX::XMMATRIX GetProjMatrixXM(bool reversedZ = false) const;
	DirectX::XMMATRIX GetViewProjMatrixXM(bool reversedZ = false) const;

	D3D12_VIEWPORT GetViewPort() const;
	
	float GetNearZ() const;
	float GetFarZ() const;
	float GetFovY() const;
	float GetAspectRatio() const;
	
	void SetFrustum(float fovY, float aspect, float nearZ, float farZ);
	void SetViewPort(const D3D12_VIEWPORT& viewPort);
	void SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

protected:
	Transform m_Transform = {};

	// 视锥体属性
	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Aspect = 0.0f;
	float m_FovY = 0.0f;

	D3D12_VIEWPORT m_ViewPort = {};

	bool m_ViewDirty = true;
};

class FirstPersonCamera :public Camera
{
public:
	FirstPersonCamera();
	~FirstPersonCamera()override;

	void SetPosition(float x, float y, float z);
	void SetPositionXM(const DirectX::XMFLOAT3& pos);

	void LookAt(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target, DirectX::XMFLOAT3 up);
	void LookTo(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT3& up);

	void Strafe(float d);
	void Walk(float d);
	void MoveForward(float d);
	void Translate(const DirectX::XMFLOAT3& dir, float magnitude);
	// 上下观察(弧度制)
	// 正rad值向上观察
	// 负rad值向下观察
	void Pitch(float rad);
	// 左右观察
	// 正rad值向右观察
	// 负rad值向左观察
	void RotateY(float rad);
};

class ThirdPersonCamera :public Camera 
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera()override;

	DirectX::XMFLOAT3 GetTargetPosition()const;
	float GetDistance()const;

	void RotateX(float rad);
	void RotateY(float rad);
	void Approach(float dist);
	// 设置初始绕X轴的弧度(注意绕x轴旋转欧拉角弧度限制在[0, pi/3])
	void SetRotationX(float rad);
	// 设置初始绕Y轴的弧度
	void SetRotationY(float rad);
	void SetTarget(const DirectX::XMFLOAT3& target);
	void SetDistance(float dist);
	void SetDistanceMinMax(float minDist, float maxDist);

private:
	DirectX::XMFLOAT3 m_Target = {};
	float m_Distance = 0.0f;
	float m_MinDist = 0.0f, m_MaxDist = 0.0f;
};


#endif // !CAMERA_H
