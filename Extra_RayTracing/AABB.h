#ifndef AABB_H
#define AABB_H

#include "common.h"

class AABB
{
public:
	AABB() {}
	AABB(const interval in_x, const interval in_y, const interval in_z) 
		:x(in_x), y(in_y), z(in_z) {}
	AABB(const point3& a, const point3& b) 
	{
		x = interval(fmin(a[0], b[0]), fmax(a[0], b[0]));
		y = interval(fmin(a[1], b[1]), fmax(a[1], b[1]));
		z = interval(fmin(a[2], b[2]), fmax(a[2], b[2]));
	}
	AABB(const AABB& box0, const AABB& box1) 
	{
		x = interval(box0.x, box1.x);
		y = interval(box0.y, box1.y);
		z = interval(box0.z, box1.z);
	}
	const interval& axis(int n)const;
	bool hit(const Ray& r, interval ray_t)const;

public:
	interval x, y, z;
};

#endif // !AABB_H
