#ifndef HITTABLE_H
#define HITTABLE_H

#include "Common/common.h"
#include "Common/AABB.h"

class Material;

class HitRecord {
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


class Hittable {
  public:
    virtual ~Hittable() = default;

    virtual bool Hit(const Ray& r, interval ray_t, HitRecord& rec) const = 0;
    virtual AABB BoundingBox() const = 0;
    virtual double PDFValue(const point3& origin, const vec3& direction) const {
        return 0.0;
    }
    virtual vec3 Random(const point3& origin) const {
        return vec3(1, 0, 0);
    }
};

class Translate : public Hittable {
public:
    Translate(shared_ptr<Hittable> object, const vec3& offset) :mObject(object), mOffset(offset) 
    {
        mBbox = mObject->BoundingBox() + offset;
    }
    bool Hit(const Ray& r, interval ray_t, HitRecord& rec) const override;
    AABB BoundingBox() const override { return  mBbox; }
private:
    shared_ptr<Hittable> mObject;
    vec3 mOffset;
    AABB mBbox;
};

class RotateY : public Hittable {
public:
    RotateY(shared_ptr<Hittable> object, double angle);
    bool Hit(const Ray& r, interval ray_t, HitRecord& rec) const override;
    AABB BoundingBox() const override { return mBbox; }

private:
    shared_ptr<Hittable> mObject;
    double mSinTheta;
    double mCosTheta;
    AABB mBbox;
};


#endif
