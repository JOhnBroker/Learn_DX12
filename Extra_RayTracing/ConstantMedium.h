#ifndef CONSTANT_MEDIUM
#define CONSTANT_MEDIUM

#include "hittable.h"
#include "material.h"
#include "Common\Texture.h"

class ConstantMedium : public Hittable
{
public:
	ConstantMedium(shared_ptr<Hittable> boundary, double density, shared_ptr<Texture> tex)
		:mBoundary(boundary), mNegInvDensity(-1 / density), mPhaseFunction(make_shared<Isotropic>(tex)) {
	}
	ConstantMedium(shared_ptr<Hittable>boundary, double density, const color& albedo)
		: mBoundary(boundary), mNegInvDensity(-1 / density), mPhaseFunction(make_shared<Isotropic>(albedo)) {
	}
	bool Hit(const Ray& r, interval ray_t, HitRecord& rec)const override;
	AABB BoundingBox()const override { return mBoundary->BoundingBox(); }
private:
	shared_ptr<Hittable> mBoundary;
	double mNegInvDensity;
	shared_ptr<Material> mPhaseFunction;
};

#endif // !CONSTANT_MEDIUM