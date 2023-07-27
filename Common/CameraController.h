
#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "Camera.h"

class CameraController
{
public:
	CameraController() = default;
	virtual ~CameraController() {}

	CameraController& operator=(const CameraController&) = delete;
	virtual void Update(float deltaTimer) = 0;
};

class FirstPersonCameraController :public CameraController 
{
public:
	~FirstPersonCameraController() override {}
	void Update(float deltaTime) override;

	void InitCamera(std::shared_ptr<FirstPersonCamera> pCamera);

	void SetMouseSensitivity(float x, float y);
	void SetMouseSpeed(float speed);
private:
	std::shared_ptr<FirstPersonCamera> m_pCamera = nullptr;

	float m_MouseSpeed = 5.0f;
	float m_MouseSensitivityX = 0.005f;
	float m_MouseSensitivityY = 0.005f;
	
	float m_CurrentYaw = 0.0f;
	float m_CurrentPitch = 0.0f;

	DirectX::XMFLOAT3 m_MoveDir{};
	float m_MoveVelocity = 0.0f;
	float m_VelocityDrag = 0.0f;
	float m_TotalDragTimeToZero = 0.25f;
	float m_DragTime = 0.0f;
};

class ThirdPersonCameraController : public CameraController 
{
public:
	~ThirdPersonCameraController() override {};
	void Update(float deltaTimer) override;

	void InitCamera(std::shared_ptr<ThirdPersonCamera> pCamera);

	void SetMouseSensitivity(float x, float y);
	void SetMouseSpeed(float speed);

private:
	std::shared_ptr<ThirdPersonCamera> m_pCamera = nullptr;

	float m_MouseSpeed = 5.0f;
	float m_MouseSensitivityX = 0.005f;
	float m_MouseSensitivityY = 0.005f;

	float m_CurrentYaw = 0.0f;
	float m_CurrentPitch = 0.0f;
};



#endif // !CAMERACONTROLLER_H