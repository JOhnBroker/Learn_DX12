#include "sphere.h"

bool sphere::hit(const Ray& r, interval ray_t, hit_record& rec) const
{
    bool res = false;
    vec3 oc = r.GetOrigin() - center;
    // 射线与球体是否相交 可以 转化为射线到球心之间的距离
    // (P(t)-C)·P(t)-C = r^2 ; P(t) = Ori + t*Dir
    // 展开公式可得
    // t^2 * Dir·Dir + 2t * Dir·(Ori - C) + (Ori - C)·(Ori - C) - r^2 = 0
    auto a = r.GetDirection().length_squared();
    auto half_b = dot(oc, r.GetDirection());
    auto c = oc.length_squared() - radius * radius;
    auto discriminant = half_b * half_b - a * c;
    auto root = (-half_b - sqrt(discriminant)) / a;
    if (discriminant < 0) goto Exit0;

    if (!ray_t.surrounds(root))
    {
        root = (-half_b + sqrt(discriminant)) / a;
        if (!ray_t.surrounds(root)) goto Exit0;
    }

    rec.t = root;
    rec.p = r.At(rec.t);
    rec.set_face_normal(r, (rec.p - center) / radius);
    rec.mat = mat;
    res = true;
Exit0:
    return res;
}