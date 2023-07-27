#include "Quaternion.h"

using namespace Math;
using namespace DirectX;

bool Quaternion::operator==(const Quaternion& q) const
{
    return FloatEqual(x, q.x) && FloatEqual(y, q.y) && FloatEqual(z, q.z) && FloatEqual(w, q.w);
}

bool Quaternion::operator!=(const Quaternion& q) const
{
    return FloatNotEqual(x, q.x) && FloatNotEqual(y, q.y) && FloatNotEqual(z, q.z) && FloatNotEqual(w, q.w);
}

Quaternion& Quaternion::operator/=(const float aScalar)
{
    x /= aScalar;
    y /= aScalar;
    z /= aScalar;
    w /= aScalar;
    return *this;
}

Quaternion& Quaternion::operator*=(const Quaternion& rhs)
{
    float tempx = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
    float tempy = w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z;
    float tempz = w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x;
    float tempw = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
    x = tempx; y = tempy; z = tempz; w = tempw;
    return *this;
}

DirectX::XMVECTOR Quaternion::ToEuler()
{
	XMVECTOR euler = QuaternionToEuler(*this) * MathHelper::Rad2Deg;

	// Makes euler angles positive 0/360 with 0.0001 hacked to support old behaviour of QuaternionToEuler
	float negativeFlip = -0.0001f * MathHelper::Rad2Deg;
	float positiveFlip = 360.0f + negativeFlip;

	XMFLOAT3 temEuler;
	XMStoreFloat3(&temEuler, euler);

	if (temEuler.x < negativeFlip)
		temEuler.x += 360.0f;
	else if (temEuler.x > positiveFlip)
		temEuler.x -= 360.0f;

	if (temEuler.y < negativeFlip)
		temEuler.y += 360.0f;
	else if (temEuler.y > positiveFlip)
		temEuler.y -= 360.0f;

	if (temEuler.z < negativeFlip)
		temEuler.z += 360.0f;
	else if (temEuler.z > positiveFlip)
		temEuler.z -= 360.0f;

	euler =  XMLoadFloat3(&temEuler);

	return euler;
}

void Quaternion::FromEular(DirectX::XMVECTOR euler)
{
	euler *= MathHelper::Deg2Rad;
	XMFLOAT3 eulerRad;
	XMStoreFloat3(&eulerRad, euler);
	*this = Internal_FromEulerRad(eulerRad);
}

Quaternion Math::Quaternion::Multi(const Quaternion& lhs, const Quaternion& rhs)
{
	return Quaternion(
		lhs.x * rhs.w + lhs.w * rhs.x + lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.y * rhs.w + lhs.w * rhs.y + lhs.z * rhs.x - lhs.x * rhs.z,
		lhs.z * rhs.w + lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x,
		lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z);
}

Quaternion Math::Quaternion::Division(const Quaternion& q, const float s)
{
	Quaternion t(q);
	return t /= s;
}

Quaternion Quaternion::Lerp(const Quaternion& q1, const Quaternion& q2, float t)
{
	Quaternion tmpQuat;
	// if (dot < 0), q1 and q2 are more than 360 deg apart.
	// The problem is that quaternions are 720deg of freedom.
	// so we - all components when lerping
	if (Dot(q1, q2) < 0.0F)
	{
		tmpQuat.Set(q1.x + t * (-q2.x - q1.x),
			q1.y + t * (-q2.y - q1.y),
			q1.z + t * (-q2.z - q1.z),
			q1.w + t * (-q2.w - q1.w));
	}
	else
	{
		tmpQuat.Set(q1.x + t * (q2.x - q1.x),
			q1.y + t * (q2.y - q1.y),
			q1.z + t * (q2.z - q1.z),
			q1.w + t * (q2.w - q1.w));
	}

	return Normalize(tmpQuat);
}

Quaternion Quaternion::Slerp(const Quaternion& q1, const Quaternion& q2, float t)
{
	Quaternion res;
	float dot = Dot(q1, q2);

	// dot = cos(theta)
	// if (dot < 0), q1 and q2 are more than 90 degrees apart,
	// so we can invert one to reduce spinning
	Quaternion temQuat;
	if (dot < 0.0f)
	{
		dot = -dot;
		temQuat.Set(-q2.x, -q2.y, -q2.w, -q2.w);
	}
	else
	{
		temQuat = q2;
	}

	if (dot < 0.95f) 
	{
		float angle = acos(dot);
		float sinadiv, sinat, sinaomt;
		sinadiv = 1.0f /sin(angle);	//分母
		sinat = sin(angle * t);
		sinaomt = sin(angle * (1.0f - t));
		temQuat.Set(
			(q1.x * sinaomt + temQuat.x * sinat) * sinadiv,
			(q1.y * sinaomt + temQuat.y * sinat) * sinadiv,
			(q1.z * sinaomt + temQuat.z * sinat) * sinadiv,
			(q1.w * sinaomt + temQuat.w * sinat) * sinadiv);
		res = temQuat;
	}
	else 
	{
		res = Lerp(q1, temQuat, t);
	}
    return res;
}

float Quaternion::Dot(const Quaternion& q1, const Quaternion& q2)
{
	return (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w);
}

float Quaternion::Magnitude(const Quaternion& q)
{
	return Sqrt(SqrMagnitude(q));
}

float Quaternion::SqrMagnitude(const Quaternion& q)
{
	return Dot(q, q);
}

Quaternion Quaternion::Normalize(const Quaternion& q)
{
	return Division(q, Magnitude(q));
}

Quaternion Quaternion::AngleAxis(float angle_deg, DirectX::XMVECTOR& anxis)
{
	Quaternion q;		//default is identity
	XMFLOAT3 temAnxis;
	XMStoreFloat3(&temAnxis, XMVector3Length(anxis));

	float angle = Deg2Rad(angle_deg);
	float mag = temAnxis.x;

	if (mag > 0.000001f) 
	{
		XMStoreFloat3(&temAnxis, anxis);
		float halfAngle = angle * 0.5f;
		float s = sinf(halfAngle) / mag;

		q.w = cosf(halfAngle);
		q.x = s * temAnxis.x;
		q.y = s * temAnxis.y;
		q.z = s * temAnxis.z;
		return q;
	}
    return q;
}

Quaternion Quaternion::Conjugate(const Quaternion& q)
{
	return Quaternion(-q.x, -q.y, -q.z, q.w);
}

Quaternion Quaternion::Inverse(const Quaternion& q)
{
	// 单位四元数 q^(-1) = q* 
	Quaternion res = Conjugate(q);
    return q;
}

float Quaternion::Angle(Quaternion a, Quaternion b)
{
	float dot = Dot(a, b);
	return IsEqualUsingDot(dot) ? 0.0f : Acos(Min(Abs(dot), 1.0f)) * 2.0f * MathHelper::Rad2Deg;
}

Quaternion Quaternion::RotateToward(Quaternion from, Quaternion to, float maxDegreesDelta)
{
	Quaternion res;
	float angle = Quaternion::Angle(from, to);
	if (FloatEqual(angle, 0.0f)) 
	{
		res = to;
	}
	else 
	{
		res = Slerp(from, to, Min(1.0f, maxDegreesDelta / angle));
	}

    return res;
}

Quaternion Quaternion::LookRotation(const DirectX::XMVECTOR& forward, const DirectX::XMVECTOR& upward)
{
	//TODO
    return Quaternion();
}

Quaternion Quaternion::Euler(const DirectX::XMVECTOR& euler)
{
	Quaternion ret;
	ret.eulerAngles = euler;
	return ret;
}

void Quaternion::Set(float inX, float inY, float inZ, float inW)
{
	x = inX;
	y = inY;
	z = inZ;
	w = inW;
}

void Quaternion::Set(const Quaternion& q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
}

void Quaternion::ToAngleAxis(float& angle, DirectX::XMFLOAT3& axis)
{
	Internal_ToAxisAngleRad(*this, axis, angle);
	angle *= MathHelper::Rad2Deg;
}

bool Math::Quaternion::IsEqualUsingDot(float dot)
{
	// Returns false in the presence of NaN values.
	return dot > 1.0f - Epsilon;
}

const Quaternion Math::Quaternion::FromToRotationUnsafe(const DirectX::XMVECTOR& from, const DirectX::XMVECTOR& to)
{
	//TODO
	return Quaternion();
}

void Math::Quaternion::Internal_ToAxisAngleRad(const Quaternion& q, DirectX::XMFLOAT3& axis, float& targetAngle)
{
	targetAngle = 2.0f * acosf(q.w);
	if (CompareApproximately(targetAngle, 0.0f)) 
	{
		axis = XMFLOAT3(1.0f, 0.0f, 0.0f);
	}
	else 
	{
		float div = 1.0f / ::sqrt(1.0f - Sqrt(q.w));
		axis.x = q.x* div;
		axis.y = q.y* div;
		axis.z = q.z* div;
	}
}

XMVECTOR Quaternion::QuaternionToEuler(const Quaternion& q, RotationOrder order)
{
    assert(fabsf(SqrMagnitude(q) - 1.0f) < 1e-5);
	int type = static_cast<int>(order);
	//setup all needed values
	float d[QuatIndexesCount] = { q.x * q.x, q.x * q.y, q.x * q.z, q.x * q.w, q.y * q.y, q.y * q.z, q.y * q.w, q.z * q.z, q.z * q.w, q.w * q.w };

	//Float array for values needed to calculate the angles
	float v[IndexesCount] = { 0.0f };
	qFunc f[3] = { qFuncs[type][0], qFuncs[type][1], qFuncs[type][2] }; //functions to be used to calculate angles

	const float SINGULARITY_CUTOFF = 0.499999f;
	XMFLOAT3 rot;
	switch (order)
	{
	case RotationOrder::OrderZYX:
		v[singularity_test] = d[xz] + d[yw];
		v[Z1] = 2.0f * (-d[xy] + d[zw]);
		v[Z2] = d[xx] - d[zz] - d[yy] + d[ww];
		v[Y1] = 1.0f;
		v[Y2] = 2.0f * v[singularity_test];
		if (Abs(v[singularity_test]) < SINGULARITY_CUTOFF)
		{
			v[X1] = 2.0f * (-d[yz] + d[xw]);
			v[X2] = d[zz] - d[yy] - d[xx] + d[ww];
		}
		else //x == xzx z == 0
		{
			float a, b, c, e;
			a = d[xz] + d[yw];
			b = -d[xy] + d[zw];
			c = d[xz] - d[yw];
			e = d[xy] + d[zw];

			v[X1] = a * e + b * c;
			v[X2] = b * e - a * c;
			f[2] = &qNull;
		}
		break;
	case RotationOrder::OrderXZY:
		v[singularity_test] = d[xy] + d[zw];
		v[X1] = 2.0f * (-d[yz] + d[xw]);
		v[X2] = d[yy] - d[zz] - d[xx] + d[ww];
		v[Z1] = 1.0f;
		v[Z2] = 2.0f * v[singularity_test];

		if (Abs(v[singularity_test]) < SINGULARITY_CUTOFF)
		{
			v[Y1] = 2.0f * (-d[xz] + d[yw]);
			v[Y2] = d[xx] - d[zz] - d[yy] + d[ww];
		}
		else //y == yxy x == 0
		{
			float a, b, c, e;
			a = d[xy] + d[zw];
			b = -d[yz] + d[xw];
			c = d[xy] - d[zw];
			e = d[yz] + d[xw];

			v[Y1] = a * e + b * c;
			v[Y2] = b * e - a * c;
			f[0] = &qNull;
		}
		break;

	case RotationOrder::OrderYZX:
		v[singularity_test] = d[xy] - d[zw];
		v[Y1] = 2.0f * (d[xz] + d[yw]);
		v[Y2] = d[xx] - d[zz] - d[yy] + d[ww];
		v[Z1] = -1.0f;
		v[Z2] = 2.0f * v[singularity_test];

		if (Abs(v[singularity_test]) < SINGULARITY_CUTOFF)
		{
			v[X1] = 2.0f * (d[yz] + d[xw]);
			v[X2] = d[yy] - d[xx] - d[zz] + d[ww];
		}
		else //x == xyx y == 0
		{
			float a, b, c, e;
			a = d[xy] - d[zw];
			b = d[xz] + d[yw];
			c = d[xy] + d[zw];
			e = -d[xz] + d[yw];

			v[X1] = a * e + b * c;
			v[X2] = b * e - a * c;
			f[1] = &qNull;
		}
		break;
	case RotationOrder::OrderZXY:
	{
		v[singularity_test] = d[yz] - d[xw];
		v[Z1] = 2.0f * (d[xy] + d[zw]);
		v[Z2] = d[yy] - d[zz] - d[xx] + d[ww];
		v[X1] = -1.0f;
		v[X2] = 2.0f * v[singularity_test];

		if (Abs(v[singularity_test]) < SINGULARITY_CUTOFF)
		{
			v[Y1] = 2.0f * (d[xz] + d[yw]);
			v[Y2] = d[zz] - d[xx] - d[yy] + d[ww];
		}
		else //x == yzy z == 0
		{
			float a, b, c, e;
			a = d[xy] + d[zw];
			b = -d[yz] + d[xw];
			c = d[xy] - d[zw];
			e = d[yz] + d[xw];

			v[Y1] = a * e + b * c;
			v[Y2] = b * e - a * c;
			f[2] = &qNull;
		}
	}
	break;
	case RotationOrder::OrderYXZ:
		v[singularity_test] = d[yz] + d[xw];
		v[Y1] = 2.0f * (-d[xz] + d[yw]);
		v[Y2] = d[zz] - d[yy] - d[xx] + d[ww];
		v[X1] = 1.0f;
		v[X2] = 2.0f * v[singularity_test];

		if (Abs(v[singularity_test]) < SINGULARITY_CUTOFF)
		{
			v[Z1] = 2.0f * (-d[xy] + d[zw]);
			v[Z2] = d[yy] - d[zz] - d[xx] + d[ww];
		}
		else //x == zyz y == 0
		{
			float a, b, c, e;
			a = d[yz] + d[xw];
			b = -d[xz] + d[yw];
			c = d[yz] - d[xw];
			e = d[xz] + d[yw];

			v[Z1] = a * e + b * c;
			v[Z2] = b * e - a * c;
			f[1] = &qNull;
		}
		break;
	case RotationOrder::OrderXYZ:
		v[singularity_test] = d[xz] - d[yw];
		v[X1] = 2.0f * (d[yz] + d[xw]);
		v[X2] = d[zz] - d[yy] - d[xx] + d[ww];
		v[Y1] = -1.0f;
		v[Y2] = 2.0f * v[singularity_test];

		if (Abs(v[singularity_test]) < SINGULARITY_CUTOFF)
		{
			v[Z1] = 2.0f * (d[xy] + d[zw]);
			v[Z2] = d[xx] - d[zz] - d[yy] + d[ww];
		}
		else //x == zxz x == 0
		{
			float a, b, c, e;
			a = d[xz] - d[yw];
			b = d[yz] + d[xw];
			c = d[xz] + d[yw];
			e = -d[yz] + d[xw];

			v[Z1] = a * e + b * c;
			v[Z2] = b * e - a * c;
			f[0] = &qNull;
		}
		break;
	}

	rot = XMFLOAT3(f[0](v[X1], v[X2]),
		f[1](v[Y1], v[Y2]),
		f[2](v[Z1], v[Z2]));

	return XMLoadFloat3(&rot);
}

Quaternion Math::Quaternion::Internal_FromEulerRad(const DirectX::XMFLOAT3& eulerAngles, RotationOrder order)
{
	float cX = cos(eulerAngles.x / 2.0f);
	float sX = sin(eulerAngles.x / 2.0f);
	float cY = cos(eulerAngles.y / 2.0f);
	float sY = sin(eulerAngles.y / 2.0f);
	float cZ = cos(eulerAngles.z / 2.0f);
	float sZ = sin(eulerAngles.z / 2.0f);

	Quaternion qX(sX, 0, 0, cX);
	Quaternion qY(0, sY, 0, cY);
	Quaternion qZ(0, 0, sZ, cZ);

	Quaternion ret;

	switch (order)
	{
	case Math::RotationOrder::OrderXYZ: CreateQuaternionFromAxisQuaternion(qX, qY, qZ, ret); break;
	case Math::RotationOrder::OrderXZY:	CreateQuaternionFromAxisQuaternion(qX, qZ, qY, ret); break;
	case Math::RotationOrder::OrderYZX:	CreateQuaternionFromAxisQuaternion(qY, qZ, qX, ret); break;
	case Math::RotationOrder::OrderYXZ:	CreateQuaternionFromAxisQuaternion(qY, qX, qZ, ret); break;
	case Math::RotationOrder::OrderZXY:	CreateQuaternionFromAxisQuaternion(qZ, qX, qY, ret); break;
	case Math::RotationOrder::OrderZYX:	CreateQuaternionFromAxisQuaternion(qZ, qY, qX, ret); break;
	}

	return ret;
}

Quaternion Math::Quaternion::CreateQuaternionFromAxisQuaternion(const Quaternion& q1, const Quaternion& q2, const Quaternion& q3, Quaternion& result)
{
	result = Multi(Multi(q1, q2), q3);
	assert(CompareApproximately(SqrMagnitude(result), 1.0F));
	return result;
}

void Math::MatrixToQuaternion(const DirectX::XMFLOAT3X3& mat, Quaternion& q)
{
	// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
	// article "Quaternionf Calculus and Fast Animation".
#if _DEBUG
	XMMATRIX _mat = XMLoadFloat3x3(&mat);
	XMFLOAT3 det;
	XMStoreFloat3(&det, XMMatrixDeterminant(_mat));
	assert(CompareApproximately(det.x, 1.0F, .005f));
#endif
	float fTrace = mat._11 + mat._22 + mat._33;
	float fRoot;

	if (fTrace > 0.0f)
	{
		// |w| > 1/2, may as well choose w > 1/2
		fRoot = std::sqrt(fTrace + 1.0f);   // 2w
		q.w = 0.5f * fRoot;
		fRoot = 0.5f / fRoot;  // 1/(4w)
		q.x = (mat._32 - mat._23) * fRoot;
		q.y = (mat._13 - mat._31) * fRoot;
		q.z = (mat._21 - mat._12) * fRoot;
	}
	else
	{
		// |w| <= 1/2
		int s_iNext[3] = { 1, 2, 0 };
		int i = 0;
		if (mat.m[1][1] > mat.m[0][0])
			i = 1;
		if (mat.m[2][2] > mat.m[i][i])
			i = 2;
		int j = s_iNext[i];
		int k = s_iNext[j];

		fRoot = std::sqrt(mat.m[i][i] - mat.m[j][j] - mat.m[k][k] + 1.0f);
		float* apkQuat[3] = { &q.x, &q.y, &q.z };
		assert(fRoot >= 0.00001f);
		*apkQuat[i] = 0.5f * fRoot;
		fRoot = 0.5f / fRoot;
		q.w = (mat.m[k][j] - mat.m[j][k]) * fRoot;
		*apkQuat[j] = (mat.m[j][i] + mat.m[i][j]) * fRoot;
		*apkQuat[k] = (mat.m[k][i] + mat.m[i][k]) * fRoot;
	}
	q = Quaternion::Normalize(q);

}
