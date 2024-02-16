#include "camera.h"
#include "interval.h"

void Camera::Render(const hittable& world)
{
	Initialize();

	std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

	for (int j = image_height - 1; j >= 0; --j)
	{
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i)
		{
			color pixel_color;
			for (int k = 0; k < samples_per_pixel; ++k)
			{
				auto u = double(i + random_double()) / (image_width - 1);
				auto v = double(j + random_double()) / (image_height - 1);
				ray r = GetRay(u, v);
				pixel_color += RayColor(r, max_depth, world);
			}
			write_color(std::cout, pixel_color, samples_per_pixel);
		}
	}
	std::clog << "\rDone.			\n";
}
void Camera::Initialize()
{
	image_height = static_cast<int>(image_width / aspect_ratio);
	image_height = (image_height < 1) ? 1 : image_height;

	m_Origin = lookfrome;

	auto theta = degrees_to_radians(vfov);
	auto tan_halftheta = tan(theta / 2);
	auto viewport_height = 2.0 * tan_halftheta * focus_dist;
	auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);

	m_W = unit_vector(lookfrome - lookat);
	m_U = unit_vector(cross(vup, m_W));
	m_V = cross(m_W, m_U);

	vec3 viewportU = viewport_width * m_U;
	vec3 viewportV = viewport_height * -m_V;

	m_PixelDeltaU = viewportU / image_width;
	m_PixelDeltaV = viewportV / image_height;

	auto viewportUpperLeft = m_Origin - (focus_dist * m_U) - viewportU / 2 - viewportV / 2;
	m_LowerLeftCorner = viewportUpperLeft + 0.5 * (m_PixelDeltaU + m_PixelDeltaV);

	auto defocusRadius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
	m_DefocusDiskU = m_U * defocusRadius;
	m_DefocusDiskV = m_V * defocusRadius;
}

ray Camera::GetRay(int i, int j)
{
	auto pixel_center = m_LowerLeftCorner + (i * m_PixelDeltaU) + (j * m_PixelDeltaV);
	auto pixel_sample = pixel_center + PixelSampleSquare();

	auto ray_origin = (defocus_angle <= 0) ? m_Origin : DefocusDiskSample();
	auto ray_direction = pixel_sample - ray_origin;
	auto ray_time = random_double();

	return ray(ray_origin, ray_direction, ray_time);
}

vec3 Camera::PixelSampleSquare() const
{
	auto px = -0.5 + random_double();
	auto py = -0.5 + random_double();
	return (px * m_PixelDeltaU) + (py * m_PixelDeltaV);
}

vec3 Camera::PixelSampleDisk(double radius) const
{
	auto p = radius * random_in_unit_disk();
	return (p[0] * m_PixelDeltaU) + (p[1] * m_PixelDeltaV);
}

point3 Camera::DefocusDiskSample() const
{
	auto p = random_in_unit_disk();
	return m_Origin + (p[0] * m_DefocusDiskU) + (p[1] * m_DefocusDiskV);
}

color Camera::RayColor(const ray& r, int depth, const hittable& world) const
{
	if (depth <= 0)
		return color(0, 0, 0);

	hit_record rec;

	if(world.hit(r,interval(0.001,infinity),rec))
	{
		ray scattered;
		color attenuation;
		if (rec.mat_ptr->Scatter(r, rec, attenuation, scattered))
			return attenuation * RayColor(scattered, depth - 1, world);
		return color(0, 0, 0);
	}
	vec3 unit_direction = unit_vector(r.GetDirection());
	auto a = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}
