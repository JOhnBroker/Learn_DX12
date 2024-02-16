#ifndef SPHERE_H
#define SPHERE_H

#include "common.h"
#include "hittable.h"


class sphere : public hittable {
  public:
    sphere(point3 _center, double _radius, shared_ptr<Material> _material)
      : center(_center), radius(_radius), mat(_material) {}

    bool hit(const Ray& r, interval ray_t, hit_record& rec) const override;

  private:
    point3 center;
    double radius;
    shared_ptr<Material> mat;
};


#endif
