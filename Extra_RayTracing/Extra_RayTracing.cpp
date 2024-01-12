#include "common.h"
#include "color.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"

#include <iostream>


color ray_color(const ray& r, const hittable& world, int depth)
{
    hit_record rec;

    if (depth <= 0) 
    {
        return color(0, 0, 0);
    }

    // t_min设为0.001是为了解决阴影粉刺
    if (world.hit(r, 0.001, infinity, rec)) 
    {
        ray scattered;
        color attenation;

        if (rec.mat_ptr->Scatter(r, rec, attenation, scattered)) 
        {
            return attenation * ray_color(scattered, world, depth - 1);
        }
        // 1. trip
        // point3 target = rec.pos + rec.normal + random_in_uint_sphere();
        // 3. 半球散射
        // point3 target = rec.pos + rec.normal + random_in_hemisphere();
        // 2. true Lambertian 
        // point3 target = rec.pos + rec.normal + random_unit_vector();
        return color(0, 0, 0);
    }
    vec3 unit_direction = unit_vector(r.GetDirection());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

int main()
{
    // Image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 800;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 30;
    const int max_depth = 50;

    // World
    hittable_list world;

    auto material_ground = make_shared<Lambertian>(color(0.8, 0.7, 0.6));
    auto material_center = make_shared<Lambertian>(color(0.7, 0.3, 0.3));
    auto material_left = make_shared<Metal>(color(0.8, 0.8, 0.8), 1.5);
    auto material_right = make_shared<Metal>(color(0.8, 0.6, 0.2), 0.5);

    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, material_ground));
    world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, material_center));
    world.add(make_shared<sphere>(point3(1, 0, -1), 0.5, material_right));
    world.add(make_shared<sphere>(point3(-1, 0, -1), 0.5, material_left));

    // Camera
    Camera camera;

    // Render
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
                ray r = camera.GetRay(u,v);
                pixel_color += ray_color(r, world, max_depth);
            }
            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }

    std::cerr << "\nDone.\n";

    return 0;
}
