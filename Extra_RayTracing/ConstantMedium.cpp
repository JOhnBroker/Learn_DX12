#include "ConstantMedium.h"

bool ConstantMedium::Hit(const Ray& r, interval ray_t, HitRecord& rec) const
{
    HitRecord rec1, rec2;

    if (!mBoundary->Hit(r, interval::universe, rec1)) {
        return false;
    }

    if (!mBoundary->Hit(r, interval(rec1.t + 0.0001, infinity), rec2)) {
        return false;
    }

    if (rec1.t < ray_t.min) rec1.t = ray_t.min;
    if (rec2.t > ray_t.max) rec2.t = ray_t.max;

    if (rec1.t > rec2.t) {
        return false;
    }

    if (rec1.t < 0) rec1.t = 0;

    auto rayLength = r.GetDirection().length();
    auto distanceInsideBoundary = (rec2.t - rec1.t) * rayLength;
    auto hitDistance = mNegInvDensity * std::log(random_double());

    if (hitDistance > distanceInsideBoundary) {
        return false;
    }

    rec.t = rec1.t + hitDistance / rayLength;
    rec.p = r.At(rec.t);

    rec.normal = vec3(1, 0, 0);
    rec.front_face = true;
    rec.mat = mPhaseFunction;

    return true;
}
