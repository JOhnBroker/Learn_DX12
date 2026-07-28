#include "hittable.h"

bool Translate::Hit(const Ray& r, interval ray_t, HitRecord& rec) const {
	Ray offset_r(r.GetOrigin() - mOffset, r.GetDirection(), r.GetTime());

	if (!mObject->Hit(offset_r, ray_t, rec)) {
		return false;
	}
	rec.p += mOffset;
	return true;
}

RotateY::RotateY(shared_ptr<Hittable> object, double angle)
	:mObject(object) {
	auto radians = degrees_to_radians(angle);
	mSinTheta = std::sin(radians);
	mCosTheta = std::cos(radians);
	mBbox = mObject->BoundingBox();

	point3 min(infinity, infinity, infinity);
	point3 max(-infinity, -infinity, -infinity);

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 2; ++j) {
			for (int k = 0; k < 2; ++k) {
				auto x = i * mBbox.x.max + (1 - i) * mBbox.x.min;
				auto y = j * mBbox.y.max + (1 - j) * mBbox.y.min;
				auto z = k * mBbox.z.max + (1 - k) * mBbox.z.min;

				auto newx = mCosTheta * x + mSinTheta * z;
				auto newz = -mSinTheta * x + mCosTheta * z;
				
				vec3 tester(newx, y, newz);

				for (int c = 0; c < 3; ++c) {
					min[c] = std::fmin(min[c], tester[c]);
					max[c] = std::fmax(max[c], tester[c]);
				}
			}
		}
	}
	mBbox = AABB(min, max);
}

bool RotateY::Hit(const Ray& r, interval ray_t, HitRecord& rec) const {
	auto origin = point3(
		(mCosTheta * r.GetOrigin().x()) - (mSinTheta * r.GetOrigin().z()),
		r.GetOrigin().y(),
		(mSinTheta * r.GetOrigin().x()) + (mCosTheta * r.GetOrigin().z())
	);

	auto direction = vec3(
		(mCosTheta * r.GetDirection().x()) - (mSinTheta * r.GetDirection().z()),
		r.GetDirection().y(),
		(mSinTheta * r.GetDirection().x()) + (mCosTheta * r.GetDirection().z())
	);

	Ray rotateR(origin, direction, r.GetTime());

	if (!mObject->Hit(rotateR, ray_t, rec)) {
		return false;
	}

	rec.p = point3(
		(mCosTheta * rec.p.x() + mSinTheta * rec.p.z()),
		rec.p.y(),
		(-mSinTheta * rec.p.x() + mCosTheta * rec.p.z())
	);

	rec.normal = vec3(
		(mCosTheta * rec.normal.x() + mSinTheta * rec.normal.z()),
		rec.normal.y(),
		(-mSinTheta * rec.normal.x() + mCosTheta * rec.normal.z())
	);
	return true;
}