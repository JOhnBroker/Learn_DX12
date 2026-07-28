#include "material.h"

bool Lambertian::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered) const
{
	// diffuse
	auto scatter_direct = rec.normal + random_unit_vector();

	if (scatter_direct.near_zero())
		scatter_direct = rec.normal;

	scattered = Ray(rec.p, scatter_direct, r_in.GetTime());
	attenuation = albedo->Value(rec.u, rec.v, rec.p);

	return true;
}

bool Lambertian::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const
{
	ONB uvw(rec.normal);
	auto scatter_direction = uvw.transform(random_cosine_direction());

	scattered = Ray(rec.p, unit_vector(scatter_direction), r_in.GetTime());
	attenuation = albedo->Value(rec.u, rec.v, rec.p);
	pdf = dot(uvw.w(), scattered.GetDirection() / pi);
	return true;
}

bool Lambertian::Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const
{
	srec.attenuation = albedo->Value(rec.u, rec.v, rec.p);
	srec.pdfPtr = make_shared<CosinePDF>(rec.normal);
	srec.skipPDF = false;
	return true;
}

double Lambertian::ScatterPDF(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const
{
	auto cosTheta = dot(rec.normal, unit_vector(scattered.GetDirection()));
	return cosTheta < 0 ? 0 : cosTheta / pi;
}

bool Metal::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered) const
{
	// specular
	vec3 reflected = reflect(unit_vector(r_in.GetDirection()), rec.normal);
	// 毛玻璃效果
	scattered = Ray(rec.p, reflected + fuzz * random_in_unit_sphere(), r_in.GetTime());
	attenuation = albedo;
	return (dot(scattered.GetDirection(), rec.normal) > 0);
}

bool Metal::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const
{
	return false;
}

bool Metal::Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const
{
	vec3 reflected = reflect(r_in.GetDirection(), rec.normal);
	reflected = unit_vector(reflected) + (fuzz * random_unit_vector());

	srec.attenuation = albedo;
	srec.pdfPtr = nullptr;
	srec.skipPDF = true;
	srec.skipPDFRay = Ray(rec.p, reflected, r_in.GetTime());

	return true;
}

bool Dielectric::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered) const
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

	scattered = Ray(rec.p, direction, r_in.GetTime());
	return true;
}

bool Dielectric::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const
{
	return false;
}

bool Dielectric::Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const
{
	srec.attenuation = color(1.0, 1.0, 1.0);
	srec.pdfPtr = nullptr;
	srec.skipPDF = true;

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

	srec.skipPDFRay = Ray(rec.p, direction, r_in.GetTime());
	return true;
}

double Dielectric::Reflectance(double cos, double refIdx)
{
	auto r0 = (1 - refIdx) / (1 + refIdx);
	r0 *= r0;
	return r0 + (1 - r0) * pow((1 - cos), 5);
}

bool Isotropic::Scatter(const Ray& r_in, const HitRecord& rec, color& attenuation, Ray& scattered, double& pdf) const
{
	scattered = Ray(rec.p, random_unit_vector(), r_in.GetTime());
	attenuation = mTex->Value(rec.u, rec.v, rec.p);
	pdf = 1 / (4 * pi);
	return true;
}

bool Isotropic::Scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const
{
	srec.attenuation = mTex->Value(rec.u, rec.v, rec.p);
	srec.pdfPtr = make_shared<SpherePDF>();
	srec.skipPDF = false;
	return true;
}

double Isotropic::ScatterPDF(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const
{
	return 1 / (4 * pi);
}
