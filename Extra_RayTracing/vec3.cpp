#include "vec3.h"

// 实现景深 散焦盘上随机点
vec3 random_in_unit_disk()
{
	while (true)
	{
		auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
		if (p.length_squared() < 1)
			return p;
	}
}

// 在球内获取一个随机点
vec3 random_in_unit_sphere()
{
	while (true)
	{
		auto p = vec3::random(-1, 1);
		if (p.length_squared() < 1)
			return p;
	}
}

// 单位球体内选随机点
vec3 random_unit_vector()
{
	return unit_vector(random_in_unit_sphere());
}

// 半球上随机点
vec3 random_on_hemisphere(const vec3& normal)
{
	vec3 on_uint_sphere = random_unit_vector();
	if (dot(on_uint_sphere, normal) > 0.0)
		return on_uint_sphere;
	else 
		return -on_uint_sphere;
}

vec3 reflect(const vec3& v, const vec3& n)
{
	return v - 2 * dot(v, n) * n;
}

vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat)
{
	auto cos_theta = fmin(dot(-uv, n), 1.0);
	vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
	vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
	return r_out_perp + r_out_parallel;
}