#ifndef HITTABLE_H
#define HITTABLE_H

#include "Common/common.h"
#include "Common/AABB.h"

class Material;

class hit_record {
  public:
    point3 p;
    vec3 normal;
    shared_ptr<Material> mat;
    double t;
    double u, v;
    bool front_face;

    void set_face_normal(const Ray& r, const vec3& outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.GetDirection(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};


class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const Ray& r, interval ray_t, hit_record& rec) const = 0;
    virtual AABB BoundingBox() const = 0;
};


#endif
