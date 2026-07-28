#ifndef Material_H
#define Material_H

#include "Common/common.h"
#include "Common/color.h"
#include "Common/Texture.h"

#include "hittable_list.h"
#include "PDF.h"


class ScatterRecord {
public:
    color attenuation;
    shared_ptr<PDF> pdfPtr;
    bool skipPDF;
    Ray skipPDFRay;
};

class Material {
  public:
    virtual ~Material() = default;

    virtual color Emitted(double u, double v, const point3& p) const {
        return color(0, 0, 0);
    }

    virtual color Emitted(const Ray& r_in, const HitRecord& rec, double u, double v, const point3& p) const {
        return color(0, 0, 0);
    }

    virtual bool Scatter(
        const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered
    ) const {
        return false;
    }

    virtual bool Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const {
        return false;
    }

    virtual bool Scatter(
        const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf
    ) const {
        return false;
    }

    virtual double ScatterPDF(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const {
        return 0;
    }
};


class Lambertian : public Material {
  public:
    Lambertian(const color& a) : albedo(make_shared<SolidColor>(a)) {}
    Lambertian(shared_ptr<Texture> a) : albedo(a) {}
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered) const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override;
    double ScatterPDF(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const override;
  private:
    shared_ptr<Texture> albedo;
};


class Metal : public Material {
public:
    Metal(const color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered) const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override;

private:
    color albedo;
    double fuzz;
};


class Dielectric : public Material {
public:
    Dielectric(double index_of_refraction) : ir(index_of_refraction) {}
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered)const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override;

private:
    double ir; // Index of Refraction

    static double Reflectance(double cosine, double ref_idx);
};


class DiffuseLight : public Material {
public:
    DiffuseLight(shared_ptr<Texture> tex) : mTex(tex) {}
    DiffuseLight(const color& emit) : mTex(make_shared<SolidColor>(emit)) {}
    color Emitted(double u, double v, const point3& p) const override {
        return mTex->Value(u, v, p);
    }
    color Emitted(const Ray& r_in, const HitRecord& rec, double u, double v, const point3& p) const override {
        if (!rec.front_face)
            return color(0, 0, 0);
        return mTex->Value(u, v, p);
    }
private:
    shared_ptr<Texture> mTex;
};


class Isotropic : public Material {
public:
    Isotropic(const color& albedo) : mTex(make_shared<SolidColor>(albedo)) {}
    Isotropic(shared_ptr<Texture> tex) : mTex(tex) {}
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenation, Ray& scattered) const override {
        scattered = Ray(rec.p, random_unit_vector(), r_in.GetTime());
        attenation = mTex->Value(rec.u, rec.v, rec.p);
        return true;
    }
    bool Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const override;
    bool Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override;
    double ScatterPDF(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const override;
private:
    shared_ptr<Texture> mTex;
};

#endif
