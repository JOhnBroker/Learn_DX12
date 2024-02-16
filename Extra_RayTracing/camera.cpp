#include "camera.h"

void Camera::Render(const hittable& world)
{
	Initialize();

	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	for (int j = 0; j < image_height; ++j) {
		std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i) {
			color pixel_color(0, 0, 0);
			for (int sample = 0; sample < samples_per_pixel; ++sample) {
				Ray r = GetRay(i, j);
				pixel_color += RayColor(r, max_depth, world);
			}
			write_color(std::cout, pixel_color, samples_per_pixel);
		}
	}

	std::clog << "\rDone.                 \n";
}
void Camera::Initialize()
{
	image_height = static_cast<int>(image_width / aspect_ratio);
	image_height = (image_height < 1) ? 1 : image_height;

	center = lookfrom;

	// Determine viewport dimensions.
	auto theta = degrees_to_radians(vfov);
	auto h = tan(theta / 2);
	auto viewport_height = 2 * h * focus_dist;
	auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);

	// Calculate the u,v,w unit basis vectors for the camera coordinate frame.
	w = unit_vector(lookfrom - lookat);
	u = unit_vector(cross(vup, w));
	v = cross(w, u);

	// Calculate the vectors across the horizontal and down the vertical viewport edges.
	vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
	vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

	// Calculate the horizontal and vertical delta vectors to the next pixel.
	pixel_delta_u = viewport_u / image_width;
	pixel_delta_v = viewport_v / image_height;

	// Calculate the location of the upper left pixel.
	auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
	pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

	// Calculate the camera defocus disk basis vectors.
	auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
	defocus_disk_u = u * defocus_radius;
	defocus_disk_v = v * defocus_radius;
}

Ray Camera::GetRay(int i, int j) const
{
	// Get a randomly-sampled camera Ray for the pixel at location i,j, originating from
		// the camera defocus disk.

	auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
	auto pixel_sample = pixel_center + PixelSampleSquare();

	auto Ray_origin = (defocus_angle <= 0) ? center : DefocusDiskSample();
	auto Ray_direction = pixel_sample - Ray_origin;

	return Ray(Ray_origin, Ray_direction);
}

vec3 Camera::PixelSampleSquare() const
{
	// Returns a random point in the square surrounding a pixel at the origin.
	auto px = -0.5 + random_double();
	auto py = -0.5 + random_double();
	return (px * pixel_delta_u) + (py * pixel_delta_v);
}

vec3 Camera::PixelSampleDisk(double radius) const
{
	// Generate a sample from the disk of given radius around a pixel at the origin.
	auto p = radius * random_in_unit_disk();
	return (p[0] * pixel_delta_u) + (p[1] * pixel_delta_v);
}

point3 Camera::DefocusDiskSample() const
{
	// Returns a random point in the camera defocus disk.
	auto p = random_in_unit_disk();
	return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
}

color Camera::RayColor(const Ray& r, int depth, const hittable& world) const
{
	// If we've exceeded the Ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	hit_record rec;

	if (world.hit(r, interval(0.001, infinity), rec)) {
		Ray scattered;
		color attenuation;
		if (rec.mat->Scatter(r, rec, attenuation, scattered))
			return attenuation * RayColor(scattered, depth - 1, world);
		return color(0, 0, 0);
	}

	vec3 unit_direction = unit_vector(r.GetDirection());
	auto a = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}
