#pragma once

#ifndef MATHHELPER_H
#define MATHHELPER_H

#include <windows.h>
#include <DirectXMath.h>
#include <cstdint>

class MathHelper
{
public:
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	static float RandF(float a,float b)
	{
		return a + RandF() * (b - a);
	}

	static int Rand(int a, int b)
	{
		return a + rand() % ((b - a)+1);
	}

	template<class T>
	static T Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	template<class T>
	static T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	template<class T>
	static T Lerp(const T& a, const T& b, float t)
	{
		return a + (b - a) * t;
	}

	template<class T>
	static T Clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x);
	}

	static float AngleFromXY(float x, float y);

	static DirectX::XMVECTOR SphericalToCartesian(float radius,float theta,float phi)
	{
		return DirectX::XMVectorSet(
			radius * sinf(phi) * cosf(theta),
			radius * cosf(phi),
			radius * sinf(phi) * sinf(theta),
			1.0f
		);
	}

	static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M)
	{
		// Inverse-transpose is just applied to normals.  So zero out 
		// translation row so that it doesn't get into our inverse-transpose
		// calculation--we don't want the inverse-transpose of the translation.

		DirectX::XMMATRIX InvM = M;
		InvM.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(InvM);
		return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, InvM));
	}

	static DirectX::XMFLOAT4X4 Identity4x4() 
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		return I;
	}

	static DirectX::XMVECTOR RandUnitVec3();
	static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);

	static const float Infinity;
	static const float Pi;



};

#endif
