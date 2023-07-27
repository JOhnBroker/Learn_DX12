#pragma once

#ifndef MATHHELPER_H
#define MATHHELPER_H

#include <windows.h>
#include <DirectXMath.h>
#include <cmath>
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
	static const float Deg2Rad;
	static const float Rad2Deg;
};

namespace Math 
{
	//indexes for pre-multiplied quaternion values
	enum QuatIndexes:int
	{
		xx,
		xy,
		xz,
		xw,
		yy,
		yz,
		yw,
		zz,
		zw,
		ww,
		QuatIndexesCount
	};

	//Indexes for values used to calculate euler angles
	enum Indexes:int
	{
		X1,		// sin / cos
		X2,		
		Y1,		
		Y2,		
		Z1,		
		Z2,		
		singularity_test,
		IndexesCount
	};

	const float Epsilon = 1.e-6f;

	static float Floor(float f) { return (float)floorf(f); }
	static float Clamp(float value, float min, float max) 
	{
		if (value < min)
			value = min;
		else if (value > max)
			value = max;
		return value;
	}
	static int Clamp(int value, int min, int max)
	{
		if (value < min)
			value = min;
		else if (value > max)
			value = max;
		return value;
	}
	static float Clamp01(float value)
	{
		if (value < 0.0f)
			return 0.0f;
		else if (value > 1.0f)
			return 1.0f;
		else
			return value;
	}
	static float Repeat(float t, float length);
	static float Sqrt(float f) { return (float)sqrt(f); }
	static float InvSqrt(float p) { return 1.0F / sqrt(p); }
	static float Acos(float f) { return (float)acos(f); }
	static float Abs(float f) { return (float)abs(f); }
	static float Min(float a, float b) { return a < b ? a : b; }
	static float Max(float a, float b) { return a > b ? a : b; }
	static bool FloatEqual(float a, float b) { return Abs(a - b) < Epsilon; }
	static bool FloatNotEqual(float a, float b) { return !FloatEqual(a, b); }
	static float LerpAngle(float a, float b, float t);
	static float Sign(float f) { return (f >= 0.0f) ? 1.0f : (-1.0f); }

	inline bool CompareApproximately(float f0, float f1, float epsilon = 0.000001F)
	{
		float dist = (f0 - f1);
		dist = Abs(dist);
		return dist <= epsilon;
	}

	inline float Deg2Rad(float deg)
	{
		// TODO : should be deg * kDeg2Rad, but can't be changed,
		// because it changes the order of operations and that affects a replay in some RegressionTests
		return deg / 360.0f * 2.0f * MathHelper::Pi;
	}

}


#endif
