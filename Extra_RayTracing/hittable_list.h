#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "Common/common.h"

#include "hittable.h"

#include <memory>
#include <vector>


class hittable_list : public hittable {
  public:
    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<hittable> object) {
        objects.push_back(object);
        bbox = AABB(bbox, object->BoundingBox());
    }

    bool hit(const Ray& r, interval ray_t, hit_record& rec) const override;
    AABB BoundingBox()const override { return bbox; }

public:
    std::vector<shared_ptr<hittable>> objects;

private:
    AABB bbox;
};


#endif
