#ifndef MATERIAL_H
#define MATERIAL_H

#include "common.h"
#include "hittable_list.h"

struct hit_record;

class Material 
{
public:
	virtual bool Scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
};

class Lambertian :public Material 
{
public:
	Lambertian(const color& a) :albedo(a) {}
	virtual bool Scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override;

private:
	color albedo;
};

class Metal :public Material 
{
public:
	Metal(const color& c, double f) :albedo(c), fuzz(f) {}
	virtual bool Scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override;

private:
	color albedo;
	double fuzz;
};

class Dielectric :public Material 
{
public:
	Dielectric(double index_of_refraction) : ref_idx(index_of_refraction) {}
	virtual bool Scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override;

private:
	static double reflectance(double cos, double ref_idx);

private:
	double ref_idx;
};

bool Lambertian::Scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const
{
	// diffuse
	auto scatter_direct = rec.normal + random_unit_vector();

	if (scatter_direct.near_zero())
		scatter_direct = rec.normal;
	scattered = ray(rec.pos, scatter_direct);
	attenuation = albedo;

	return true;
}

bool Metal::Scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const
{
	// specular
	auto refected = reflect(unit_vector(r_in.GetDirection()), rec.normal);
	// 毛玻璃效果
	scattered = ray(rec.pos, refected + fuzz * random_in_uint_sphere());
	attenuation = albedo;
	return (dot(scattered.GetDirection(), rec.normal) > 0);
}

bool Dielectric::Scatter(const ray & r_in, const hit_record & rec, color & attenuation, ray & scattered) const
{
	attenuation = color(1.0, 1.0, 1.0);
	double refraction_ratio = rec.front_face ? (1.0 / ref_idx) : ref_idx;
	vec3 uint_direction = unit_vector(r_in.GetDirection());

	double cos_theta = fmin(dot(-uint_direction, rec.normal), 1.0);
	double sin_theta = sqrt(1 - cos_theta * cos_theta);
	bool isReflect = refraction_ratio * sin_theta > 1.0;
	vec3 direction;

	if (isReflect || reflectance(cos_theta, ref_idx) > random_double()) 
		direction = reflect(uint_direction, rec.normal);
	else
		direction = refract(uint_direction, rec.normal, refraction_ratio);

	scattered = ray(rec.pos, direction);
	return true;
}

inline double Dielectric::reflectance(double cos, double refIdx)
{
	auto r0 = (1 - refIdx) / (1 + refIdx);
	r0 *= r0;
	return r0 + (1 - r0) * pow((1 - cos), 5);
}

#endif // !MATERIAL_H
