#include "AABB.h"

const interval& AABB::axis(int n) const
{
	if (n == 1) return y;
	if (n == 2)return z;
	return x;
}

bool AABB::hit(const Ray& r, interval ray_t) const
{
	for (int a = 0; a < 3; ++a)
	{
		auto invD = 1 / r.GetDirection()[a];
		auto orig = r.GetOrigin()[a];

		// 射线方程 P(t) = Ori + t * Dir
		// 在某一轴上的交点为 x/y/z 
		auto t0 = (axis(a).min - orig) * invD;
		auto t1 = (axis(a).max - orig) * invD;

		if (invD < 0)std::swap(t0, t1);

		if (t0 > ray_t.min) ray_t.min = t0;
		if (t1 < ray_t.max) ray_t.max = t1;

		if (ray_t.max <= ray_t.min) return false;
	}
	return true;
}
