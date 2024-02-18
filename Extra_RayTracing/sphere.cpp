#include "sphere.h"

bool sphere::hit(const Ray& r, interval ray_t, hit_record& rec) const
{
    bool res = false;
    point3 cen = is_moving ? GetCenter(r.GetTime()) : center;
    vec3 oc = r.GetOrigin() - cen;
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
    rec.set_face_normal(r, (rec.p - cen) / radius);
    GetSphereUV((rec.p - cen) / radius, rec.u, rec.v);
    rec.mat = mat;
    res = true;
Exit0:
    return res;
}

void sphere::GetSphereUV(const point3& p, double& u, double& v)
{
    // p: a given point on the sphere of radius one, centered at the origin.
    // u: returned value [0,1] of angle around the Y axis from X=-1.
    // v: returned value [0,1] of angle from Y=-1 to Y=+1.
    //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
    //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
    //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

    auto theta = acos(-p.y());
    auto phi = atan2(-p.z(), p.x()) + pi;

    u = phi / (2 * pi);
    v = theta / pi;
}
