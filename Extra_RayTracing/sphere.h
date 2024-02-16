#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"

class sphere :public hittable
{
public:
    // Stationary Sphere
    sphere(point3 cen, double r, shared_ptr<Material> mat) :origin(cen), 
        radius(r), mat_ptr(mat), is_moving(false) {}

    // Moving Sphere
    sphere(point3 cen1, point3 cen2, double r, shared_ptr<Material> mat)
        :origin(cen1), radius(r), mat_ptr(mat), is_moving(true)
    {
        center_vec = cen2 - cen1;
    }

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec)const override;

public:
    point3 origin = point3(0, 0, 0);
    double radius = 0.0f;
    shared_ptr<Material> mat_ptr;

    bool is_moving;
    vec3 center_vec;

    point3 GetCenter(double t)const 
    {
        // linear
        return origin + t * center_vec;
    }

};

bool sphere::hit(const ray& r, interval ray_t, hit_record& rec) const
{
    bool res = false;
    point3  center = is_moving ? GetCenter(r.GetTime()) : origin;
    vec3 oc = r.GetOrigin() - center;
    // �����������Ƿ��ཻ ���� ת��Ϊ���ߵ�����֮��ľ���
    // (P(t)-C)��P(t)-C = r^2 ; P(t) = Ori + t*Dir
    // չ����ʽ�ɵ�
    // t^2 * Dir��Dir + 2t * Dir��(Ori - C) + (Ori - C)��(Ori - C) - r^2 = 0
    auto a = r.GetDirection().length_squared();
    auto half_b = dot(oc, r.GetDirection());
    auto c = oc.length_squared()- radius * radius;
    auto discriminant = half_b * half_b - a * c;
    auto root = (-half_b - sqrt(discriminant)) / a;
    if (discriminant < 0) goto Exit0;

    if (!ray_t.surrounds(root))
    {
        root = (-half_b + sqrt(discriminant)) / a;
        if (!ray_t.surrounds(root)) goto Exit0;
    }

    rec.t = root;
    rec.pos = r.At(root);
    rec.set_face_normal(r, (rec.pos - center) / radius);
    rec.mat_ptr = mat_ptr;
    res = true;
Exit0:
    return res;
}

#endif // !SPHERE_H
