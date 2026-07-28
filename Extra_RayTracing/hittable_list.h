#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "Common/common.h"

#include "hittable.h"

#include <memory>
#include <vector>


class hittable_list : public Hittable {
  public:
    hittable_list() {}
    hittable_list(shared_ptr<Hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<Hittable> object) {
        objects.push_back(object);
        bbox = AABB(bbox, object->BoundingBox());
    }

    bool Hit(const Ray& r, interval ray_t, HitRecord& rec) const override;
    AABB BoundingBox()const override { return bbox; }

public:
    std::vector<shared_ptr<Hittable>> objects;

private:
    AABB bbox;
};


#endif
