#include "hittable_list.h"

bool hittable_list::Hit(const Ray & r, interval ray_t, HitRecord & rec) const 
{
    HitRecord temp_rec;
    auto hit_anything = false;
    auto closest_so_far = ray_t.max;

    for (const auto& object : objects) {
        if (object->Hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}