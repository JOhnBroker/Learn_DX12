#include "material.h"

bool Lambertian::Scatter(const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered) const
{
	// diffuse
	auto scatter_direct = rec.normal + random_unit_vector();

	if (scatter_direct.near_zero())
		scatter_direct = rec.normal;

	//scattered = Ray(rec.p, scatter_direct, r_in.GetTime());
	scattered = Ray(rec.p, scatter_direct);
	attenuation = albedo;

	return true;
}

bool Metal::Scatter(const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered) const
{
	// specular
	vec3 reflected = reflect(unit_vector(r_in.GetDirection()), rec.normal);
	// 毛玻璃效果
	//scattered = Ray(rec.p, refected + fuzz * random_in_unit_sphere(), r_in.GetTime());
	scattered = Ray(rec.p, reflected + fuzz * random_in_unit_sphere());
	attenuation = albedo;
	return (dot(scattered.GetDirection(), rec.normal) > 0);
}

bool Dielectric::Scatter(const Ray& r_in, const hit_record& rec, color& attenuation, Ray& scattered) const
{
	attenuation = color(1.0, 1.0, 1.0);
	double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;
	vec3 unit_direction = unit_vector(r_in.GetDirection());

	double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
	double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	bool isReflect = refraction_ratio * sin_theta > 1.0;
	vec3 direction;

	if (isReflect || Reflectance(cos_theta, refraction_ratio) > random_double())
		direction = reflect(unit_direction, rec.normal);
	else
		direction = refract(unit_direction, rec.normal, refraction_ratio);

	//scattered = Ray(rec.p, direction, r_in.GetTime());
	scattered = Ray(rec.p, direction);
	return true;
}

double Dielectric::Reflectance(double cos, double refIdx)
{
	auto r0 = (1 - refIdx) / (1 + refIdx);
	r0 *= r0;
	return r0 + (1 - r0) * pow((1 - cos), 5);
}
