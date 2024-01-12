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
	// Ã«²£Á§Ð§¹û
	scattered = ray(rec.pos, refected + fuzz * random_in_uint_sphere());
	attenuation = albedo;
	return (dot(scattered.GetDirection(), rec.normal) > 0);
}

#endif // !MATERIAL_H
