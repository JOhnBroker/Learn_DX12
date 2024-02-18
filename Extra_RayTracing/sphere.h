#ifndef SPHERE_H
#define SPHERE_H

#include "common.h"
#include "hittable.h"


class sphere : public hittable {
  public:
    // Stationary Sphere
    sphere(point3 _center, double _radius, shared_ptr<Material> _material)
      : center(_center), radius(_radius), mat(_material), is_moving(false) 
    {
        auto rvec = vec3(radius, radius, radius);
        bbox = AABB(center - rvec, center + rvec);
    }
    // Moving Sphere
    sphere(point3 _center1, point3 _center2, double _radius, shared_ptr<Material> _material)
        : center(_center1), radius(_radius), mat(_material), is_moving(true) 
    {
        auto rvec = vec3(radius, radius, radius);
        auto box1 = AABB(_center1 - rvec, _center1 + rvec);
        auto box2 = AABB(_center2 - rvec, _center2 + rvec);
        bbox = AABB(box1, box2);

        center_vec = _center2 - _center1;
    }

    bool hit(const Ray& r, interval ray_t, hit_record& rec) const override;

    AABB BoundingBox()const override { return bbox; }

  private:
    point3 center;
    double radius;
    shared_ptr<Material> mat;
    bool is_moving;
    vec3 center_vec;
    AABB bbox;

    point3 GetCenter(double t)const 
    {
        // linear
        return center + t * center_vec;
    }

    static void GetSphereUV(const point3& p, double& u, double& v);

};


#endif
