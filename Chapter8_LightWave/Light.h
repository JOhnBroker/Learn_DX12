#ifndef LIGHT_H
#define LIGHT_H

#include "d3dUtil.h"

#define MAXLIGHTNUM 16

struct Light 
{
	DirectX::XMFLOAT3 m_STRENGTH = { 0.5f,0.5f,0.5f };
	float m_FalloffStart = 1.0f;
	DirectX::XMFLOAT3 m_Direction = { 0.0f,-1.0f,0.0f };
	float m_FalloffEnd = 10.0f;
	DirectX::XMFLOAT3 m_Position = { 0.0f,0.0f,0.0f };
	float m_SpotPower = 64.0f;
};

#endif
