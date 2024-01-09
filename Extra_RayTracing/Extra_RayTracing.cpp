#include "color.h"
#include "ray.h"

#include <iostream>

double hit_sphere(const point3& center, double radius, const ray& r) 
{
    vec3 oc = r.GetOrigin() - center;
    // 射线与球体是否相交 可以 转化为射线到球心之间的距离
    // (P(t)-C)·P(t)-C = r^2 ; P(t) = Ori + t*Dir
    // 展开公式可得
    // t^2 * Dir·Dir + 2t * Dir·(Ori - C) + (Ori - C)·(Ori - C) - r^2 = 0
    auto a = dot(r.GetDirection(), r.GetDirection());
    auto half_b = dot(r.GetDirection(), oc);
    auto c = dot(oc, oc) - radius * radius;
    auto discriminant = half_b * half_b - a * c;
    if (discriminant < 0)
        return -1;
    else
        return (-half_b - sqrt(discriminant)) / a;
}

color ray_color(const ray& r) 
{
    auto t = hit_sphere(point3(0, 0, -1), 0.5, r);
    if (t > 0.0) 
    {
        vec3 N = unit_vector(r.At(t) - vec3(0, 0, -1));
        return 0.5 * color(N.x() + 1, N.y() + 1, N.z() + 1);
    }
    vec3 unit_direction = unit_vector(r.GetDirection());
    t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

int main()
{
    // Image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);

    // Camera
    auto viewport_height = 2.0f;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;

    auto origin = point3(0, 0, 0);
    auto horizontal = vec3(viewport_width, 0, 0);
    auto vertical = vec3(0, viewport_height, 0);
    auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);

    // Render
    std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) 
    {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) 
        {
            auto u = double(i) / (image_width - 1);
            auto v = double(j) / (image_height - 1);
            ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
            color pixel_color = ray_color(r);
            write_color(std::cout, pixel_color);
        }
    }

    std::cerr << "\nDone.\n";

    return 0;
}
