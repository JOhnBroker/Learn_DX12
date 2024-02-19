#ifndef Material_H
#define Material_H

#include "Common/common.h"
#include "Common/color.h"
#include "Common/Texture.h"

#include "hittable_list.h"


class Material {
  public:
    virtual ~Material() = default;

    virtual bool Scatter(
        const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered
    ) const = 0;
};


class Lambertian : public Material {
  public:
    Lambertian(const color& a) : albedo(make_shared<SolidColor>(a)) {}
    Lambertian(shared_ptr<Texture> a) : albedo(a) {}
    bool Scatter(const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered) const override;

  private:
    shared_ptr<Texture> albedo;
};


class Metal : public Material {
public:
    Metal(const color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}
    bool Scatter(const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered) const override;

private:
    color albedo;
    double fuzz;
};


class Dielectric : public Material {
public:
    Dielectric(double index_of_refraction) : ir(index_of_refraction) {}
    bool Scatter(const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered)const override;
private:
    double ir; // Index of Refraction

    static double Reflectance(double cosine, double ref_idx);
};


#endif
