#include "camera.h"
#include "PDF.h"

void Camera::Render(const Hittable& world, const Hittable& lights)
{
	Initialize();

	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	for (int j = 0; j < image_height; ++j) {
		std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i) {
			color pixel_color(0, 0, 0);
			for (int s_j = 0; s_j < sqrt_spp; ++s_j) {
				for (int s_i = 0; s_i < sqrt_spp; ++s_i) {
					Ray r = GetRay(i, j, s_i, s_j);
					pixel_color += RayColor(r, max_depth, world, lights);
				}
			}
			write_color(std::cout, pixel_samples_scale * pixel_color);
		}
	}

	std::clog << "\rDone.                 \n";
}
void Camera::Initialize()
{
	image_height = static_cast<int>(image_width / aspect_ratio);
	image_height = (image_height < 1) ? 1 : image_height;

	sqrt_spp = int(std::sqrt(samples_per_pixel));
	pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
	recip_sqrt_spp = 1.0 / sqrt_spp;

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

Ray Camera::GetRay(int i, int j, int s_i, int s_j) const
{
	// Get a randomly-sampled camera Ray for the pixel at location i,j, originating from
	// the camera defocus disk.

	auto offset = SampleSquareStratified(s_i, s_j);
	auto pixel_center = pixel00_loc
		+ ((i + offset.x()) * pixel_delta_u)
		+ ((j + offset.y()) * pixel_delta_v);
	auto pixel_sample = pixel_center + PixelSampleSquare();

	auto ray_origin = (defocus_angle <= 0) ? center : DefocusDiskSample();
	auto ray_direction = pixel_sample - ray_origin;
	auto ray_time = random_double();

	return Ray(ray_origin, ray_direction, ray_time);
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

color Camera::RayColor(const Ray& r, int depth, const Hittable& world, const Hittable& lights) const
{
	// If we've exceeded the Ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	HitRecord rec;

	// If the ray hits nothing, return the background color.
	if (!world.Hit(r, interval(0.001, infinity), rec)) {
		return background;
	}
	ScatterRecord srec;
	color colorFromEmission = rec.mat->Emitted(r, rec, rec.u, rec.v, rec.p);

	if (!rec.mat->Scatter(r, rec, srec))
		return colorFromEmission;
	
	if (srec.skipPDF) {
		return srec.attenuation * RayColor(srec.skipPDFRay, depth - 1, world, lights);
	}

	auto lightPtr = make_shared<HittablePDF>(lights, rec.p);
	MixturePDF mixedPDF(lightPtr, srec.pdfPtr);

	Ray scattered;
	scattered = Ray(rec.p, mixedPDF.Generate(), r.GetTime());
	auto pdf_value = mixedPDF.Value(scattered.GetDirection());

	double scatteringPDF = rec.mat->ScatterPDF(r, rec, scattered);

	color sampleColor = RayColor(scattered, depth - 1, world, lights);
	color colorFromScatter = 
		(srec.attenuation * scatteringPDF * sampleColor)
		/ pdf_value;

	return colorFromEmission + colorFromScatter;
}

vec3 Camera::SampleSquareStratified(int s_i, int s_j) const {
	// Returns the vector to a random point in the square sub-pixel specified by grid
	// indices s_i and s_j, for an idealized unit square pixel [-.5,-.5] to [+.5,+.5].

	auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
	auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

	return vec3(px, py, 0);
}