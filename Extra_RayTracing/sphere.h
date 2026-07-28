#ifndef SPHERE_H
#define SPHERE_H

#include "Common/common.h"
#include "hittable.h"
#include "ONB.h"


class sphere : public Hittable {
  public:
    // Stationary Sphere
      sphere(const point3& _center, double _radius, shared_ptr<Material> _material)
          : center(_center, vec3(0, 0, 0), 0), radius(std::fmax(0, _radius)), mat(_material)
    {
        auto rvec = vec3(radius, radius, radius);
        bbox = AABB(_center - rvec, _center + rvec);
    }
    // Moving Sphere
      sphere(const point3& _center1, const point3& _center2, double _radius, shared_ptr<Material> _material)
          : center(_center1, _center2 - _center1, 0), radius(std::fmax(0, _radius)), mat(_material)
    {
        auto rvec = vec3(radius, radius, radius);
        auto box1 = AABB(center.At(0) - rvec, center.At(0) + rvec);
        auto box2 = AABB(center.At(1) - rvec, center.At(1) + rvec);
        bbox = AABB(box1, box2);
    }

    bool Hit(const Ray& r, interval ray_t, HitRecord& rec) const override;

    AABB BoundingBox()const override { return bbox; }

    double PDFValue(const point3& origin, const vec3& direction) const override;
    vec3 Random(const point3& origin) const override;

  private:
    Ray center;
    double radius;
    shared_ptr<Material> mat;
    AABB bbox;

    static void GetSphereUV(const point3& p, double& u, double& v);

    static vec3 RandomToSphere(double radius, double distanceSquared);
};


#endif
