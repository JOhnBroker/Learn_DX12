#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"

class sphere :public hittable
{
public:
	sphere() {}
    sphere(point3 cen, double r, shared_ptr<Material> mat) :center(cen), radius(r), mat_ptr(mat) {}

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec)const override;

public:
    point3 center = point3(0, 0, 0);
    double radius = 0.0f;
    shared_ptr<Material> mat_ptr;
};

bool sphere::hit(const ray& r, double t_min, double t_max, hit_record& rec) const
{
    bool res = false;
    vec3 oc = r.GetOrigin() - center;
    // �����������Ƿ��ཻ ���� ת��Ϊ���ߵ�����֮��ľ���
    // (P(t)-C)��P(t)-C = r^2 ; P(t) = Ori + t*Dir
    // չ����ʽ�ɵ�
    // t^2 * Dir��Dir + 2t * Dir��(Ori - C) + (Ori - C)��(Ori - C) - r^2 = 0
    auto a = dot(r.GetDirection(), r.GetDirection());
    auto half_b = dot(r.GetDirection(), oc);
    auto c = dot(oc, oc) - radius * radius;
    auto discriminant = half_b * half_b - a * c;
    auto root = (-half_b - sqrt(discriminant)) / a;
    if (discriminant < 0) goto Exit0;

    if (root < t_min || root > t_max)
    {
        root = (-half_b + sqrt(discriminant)) / a;
        if (root < t_min || root > t_max) goto Exit0;
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
