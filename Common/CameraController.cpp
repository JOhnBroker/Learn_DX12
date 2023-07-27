#include "CameraController.h"
#include <imgui.h>

using namespace DirectX;

void FirstPersonCameraController::Update(float deltaTime)
{
	ImGuiIO& io = ImGui::GetIO();
	
	float yaw = 0.0f, pitch = 0.0f;
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) 
	{
		yaw += io.MouseDelta.x * m_MouseSensitivityX;
		pitch += io.MouseDelta.y * m_MouseSensitivityY;
	}

	int forward = (
		(ImGui::IsKeyDown(ImGuiKey_W) ? 1 : 0) +
		(ImGui::IsKeyDown(ImGuiKey_S) ? -1 : 0)
		);
	int strafe = (
		(ImGui::IsKeyDown(ImGuiKey_A) ? -1 : 0) +
		(ImGui::IsKeyDown(ImGuiKey_D) ? 1 : 0)
		);

	if (forward || strafe)
	{
		XMVECTOR dir = 
			m_pCamera->GetLookAxisXM() * (float)forward +
			m_pCamera->GetRightAxisXM() * (float)strafe;
		XMStoreFloat3(&m_MoveDir, dir);
		m_MoveVelocity = m_MouseSpeed;
		m_DragTime = m_TotalDragTimeToZero;
		m_VelocityDrag = m_MouseSpeed / m_DragTime;
	}
	else 
	{
		if (m_DragTime > 0.0f) 
		{
			m_DragTime -= deltaTime;
			m_MoveVelocity -= m_MoveVelocity * deltaTime;
		}
		else 
		{
			m_MoveVelocity = 0.0f;
		}
	}

	m_pCamera->RotateY(yaw);
	m_pCamera->Pitch(pitch);

	m_pCamera->Translate(m_MoveDir, m_MoveVelocity * deltaTime);
}

void FirstPersonCameraController::InitCamera(std::shared_ptr<FirstPersonCamera> pCamera)
{
	m_pCamera = pCamera;
}

void FirstPersonCameraController::SetMouseSensitivity(float x, float y)
{
	m_MouseSensitivityX = x;
	m_MouseSensitivityY = y;
}

void FirstPersonCameraController::SetMouseSpeed(float speed)
{
	m_MouseSpeed = speed;
}

void ThirdPersonCameraController::Update(float deltaTimer)
{
	ImGuiIO& io = ImGui::GetIO();

	float yaw = 0.0f, pitch = 0.0f;
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		yaw += io.MouseDelta.x * m_MouseSensitivityX;
		pitch += io.MouseDelta.y * m_MouseSensitivityY;
	}

	m_pCamera->RotateX(pitch);
	m_pCamera->RotateY(yaw);

	m_pCamera->Approach(-io.MouseWheel * m_MouseSpeed);
}

void ThirdPersonCameraController::InitCamera(std::shared_ptr<ThirdPersonCamera> pCamera)
{
	m_pCamera = pCamera;
}

void ThirdPersonCameraController::SetMouseSensitivity(float x, float y)
{
	m_MouseSensitivityX = x;
	m_MouseSensitivityY = y;
}

void ThirdPersonCameraController::SetMouseSpeed(float speed)
{
	m_MouseSpeed = speed;
}
