#ifndef QUAD_H
#define QUAD_H
#include "hittable.h"
#include "hittable_list.h"


class Quad : public Hittable
{
public:
	Quad(const point3& q, const vec3& u, const vec3& v, shared_ptr<Material> mat);

	virtual void SetBoundingBox()
	{
		auto bboxDiagonal1 = AABB(mQ, mQ + mU + mV);
		auto bboxDiagonal2 = AABB(mQ + mU, mQ + mV);
		mBbox = AABB(bboxDiagonal1, bboxDiagonal2);

	}

	AABB BoundingBox() const override
	{
		return mBbox;
	}

	bool Hit(const Ray& r, interval ray_t, HitRecord& rec) const override;

	virtual bool isInterior(double a, double b, HitRecord& rec) const;

	double PDFValue(const point3& origin, const vec3& direction) const override;
	vec3 Random(const point3& origin) const override;

private:
	point3 mQ;
	vec3 mU, mV;
	vec3 mW;
	shared_ptr<Material> mMat;
	AABB mBbox;
	vec3 mNormal;
	double mD;
	double mArea;
};

inline shared_ptr<hittable_list> Box(const point3& a, const point3& b, shared_ptr<Material> mat)
{
	auto sides = make_shared<hittable_list>();

	auto min = point3(std::fmin(a.x(), b.x()), std::fmin(a.y(), b.y()), std::fmin(a.z(), b.z()));
	auto max = point3(std::fmax(a.x(), b.x()), std::fmax(a.y(), b.y()), std::fmax(a.z(), b.z()));

	auto dx = vec3(max.x() - min.x(), 0, 0);
	auto dy = vec3(0, max.y() - min.y(), 0);
	auto dz = vec3(0, 0, max.z() - min.z());

	sides->add(make_shared<Quad>(point3(min.x(), min.y(), max.z()), dx, dy, mat)); // front
	sides->add(make_shared<Quad>(point3(max.x(), min.y(), max.z()), -dz, dy, mat)); // right
	sides->add(make_shared<Quad>(point3(max.x(), min.y(), min.z()), -dx, dy, mat)); // back
	sides->add(make_shared<Quad>(point3(min.x(), min.y(), min.z()), dz, dy, mat)); // left
	sides->add(make_shared<Quad>(point3(min.x(), max.y(), max.z()), dx, -dz, mat)); // top
	sides->add(make_shared<Quad>(point3(min.x(), min.y(), min.z()), dx, dz, mat)); // bottom

	return sides;
}

#endif