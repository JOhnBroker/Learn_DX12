#include "MathHelper.h"
#include <float.h>

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = 3.1415926535f;
const float MathHelper::Deg2Rad = MathHelper::Pi / 180.0f;
const float MathHelper::Rad2Deg = 1.0f / MathHelper::Deg2Rad;

float MathHelper::AngleFromXY(float x, float y)
{
	float theta = 0.0f;

	//TODO:function not complete

	return 0.0f;
}

XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);

	while (true) 
	{
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;
		return XMVector3Normalize(v);
	}
}

DirectX::XMVECTOR MathHelper::RandHemisphereUnitVec3(XMVECTOR n)
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	while (true) 
	{
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		if (XMVector3Less(XMVector3Dot(n, v), Zero))
			continue;

		return XMVector3Normalize(v);
	}
}

float Math::Repeat(float t, float length)
{
	return Clamp(t - Math::Floor(t / length) * length, 0.0f, length);
}

float Math::LerpAngle(float a, float b, float t)
{
	float delta = Repeat((b - a), 360);
	if (delta > 180)
		delta -= 360;
	return a + delta * Clamp01(t);
}
