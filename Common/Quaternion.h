#pragma once
#ifndef QUATERNION_H
#define QUATERNION_H

#include <DirectXMath.h>
#include "MathHelper.h"

namespace Math 
{

	enum class RotationOrder 
	{
		OrderXYZ,
		OrderXZY,
		OrderYZX,
		OrderYXZ,
		OrderZXY,
		OrderZYX,
		RotationOrderCount,
		OrderUnityDefault = OrderZXY
	};

	inline float qAsin(float a, float b) 
	{
		return a * asin(Clamp(b, -1.0f, 1.0f));
	}

	inline float qAtan2(float a, float b)
	{
		return atan2(a, b);
	}
	inline float qNull(float a, float b)
	{
		a ; b;
		return 0;
	}

	typedef float (*qFunc)(float, float);

	// 四元数转欧拉角
	static qFunc qFuncs[(int)RotationOrder::RotationOrderCount][3] =
	{
		{&qAtan2, &qAsin, &qAtan2},     //OrderXYZ
		{&qAtan2, &qAtan2, &qAsin},     //OrderXZY
		{&qAtan2, &qAtan2, &qAsin},     //OrderYZX,
		{&qAsin, &qAtan2, &qAtan2},     //OrderYXZ,
		{&qAsin, &qAtan2, &qAtan2},     //OrderZXY,
		{&qAtan2, &qAsin, &qAtan2},     //OrderZYX,
	};

	class Quaternion
	{
	public:
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 1.0f;

	public:
		Quaternion() :x(0.0f), y(0.0f), z(0.0f), w(1.0f) {};
		Quaternion(float _x, float _y, float _z, float _w) :x(_x), y(_y), z(_z), w(_w) {};

		bool operator==(const Quaternion& q)const;
		bool operator!=(const Quaternion& q)const;
		Quaternion& operator/=(const float aScalar);
		Quaternion& operator*=(const Quaternion& rhs);

		DirectX::XMVECTOR ToEuler();
		void FromEular(DirectX::XMVECTOR euler);

		DirectX::XMVECTOR GetRotationXM() { return DirectX::XMLoadFloat4(&m_Quat); }
		DirectX::XMFLOAT4 GetRotation() { return m_Quat; }
		static Quaternion Multi(const Quaternion& lhs, const Quaternion& rhs);
		static Quaternion Division(const Quaternion& q, const float s);
		static Quaternion Lerp(const Quaternion& q1, const Quaternion& q2, float t);
		static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);
		static float Dot(const Quaternion& q1, const Quaternion& q2);
		// 四元数的模
		static float Magnitude(const Quaternion& q);
		// 四元数模的平方
		static float SqrMagnitude(const Quaternion& q);
		static Quaternion Normalize(const Quaternion& q);
		// 极坐标表示法(弧度制)
		static Quaternion AngleAxis(float angle_deg, DirectX::XMVECTOR& anxis);
		// 共轭
		static Quaternion Conjugate(const Quaternion& q);
		// 求逆
		static Quaternion Inverse(const Quaternion& q);
		static float Angle(Quaternion a, Quaternion b);
		static Quaternion RotateToward(Quaternion from, Quaternion to, float maxDegreesDelta);
		static Quaternion LookRotation(const DirectX::XMVECTOR& forward, const DirectX::XMVECTOR& upward);
		static Quaternion Euler(const DirectX::XMVECTOR& euler);


		void Set(float inX, float inY, float inZ, float inW);
		void Set(const Quaternion& q);
		void Set(const float* array) { x = array[0], y = array[1], z = array[2], w = array[3]; }
		void ToAngleAxis(float& angle, DirectX::XMFLOAT3& axis);

		__declspec(property(get = ToEuler, put = FromEular)) DirectX::XMVECTOR eulerAngles;

	private:
		DirectX::XMFLOAT4 m_Quat{ x, y, z, w };

		static bool IsEqualUsingDot(float dot);
		static const Quaternion FromToRotationUnsafe(const DirectX::XMVECTOR& from, const DirectX::XMVECTOR& to);
		static void Internal_ToAxisAngleRad(const Quaternion& q, DirectX::XMFLOAT3& axis, float& targetAngle);
		// Generates an orthornormal basis from a look at rotation, returns if it was successful
		// (Righthanded)
		//static bool LookRotationToMatrix(const )
		static DirectX::XMVECTOR QuaternionToEuler(const Quaternion& q, RotationOrder order = RotationOrder::OrderUnityDefault);
		static Quaternion Internal_FromEulerRad(const DirectX::XMFLOAT3& eulerAngles, RotationOrder order = RotationOrder::OrderUnityDefault);
		static Quaternion CreateQuaternionFromAxisQuaternion(const Quaternion& q1, const Quaternion& q2, const Quaternion& q3, Quaternion& result);

	};

	void MatrixToQuaternion(const DirectX::XMFLOAT3X3& mat, Quaternion& q);

}
#endif // !QUATERNION_H
