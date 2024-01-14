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

hittable_list random_scene() 
{
    hittable_list world;

    auto ground_maerial = make_shared<Lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_maerial));

    double r = 0.2;
    for (int i = -11; i < 11; ++i) 
    {
        for (int j = -11; j < 11; ++j) 
        {
            auto choose_mat = random_double();
            point3 center(i + 0.9 * random_double(), 0.2, j + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) 
            {
                shared_ptr<Material> material;
                if (choose_mat < 0.8) 
                {
                    //diffuse
                    auto albedo = color::random() * color::random();
                    material = make_shared<Lambertian>(albedo);
                }
                else if (choose_mat < 0.95) 
                {
                    auto albedo = color::random() * color::random();
                    auto fuzz = random_double(0, 0.5);
                    material = make_shared<Metal>(albedo, fuzz);
                }
                else 
                {
                    // glass
                    material = make_shared<Dielectric>(1.5);
                }
                world.add(make_shared<sphere>(center, r, material));
            }
        }
    }

    auto material1 = make_shared<Dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<Lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<Metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}


int main()
{
    // Image
    const auto aspect_ratio = 3.0 / 2.0;
    const int image_width = 120;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 30;
    const int max_depth = 25;

    // World
    //hittable_list world;
    //
    //auto material_ground = make_shared<Lambertian>(color(0.8, 0.8, 0.0));
    //auto material_center = make_shared<Lambertian>(color(0.1, 0.2, 0.5));
    //auto material_left = make_shared<Dielectric>(1.5);                      //glass
    //auto material_right = make_shared<Metal>(color(0.8, 0.6, 0.2), 0.5);
    //
    //world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, material_ground));
    //world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, material_center));
    //world.add(make_shared<sphere>(point3(1, 0, -1), 0.5, material_right));
    //world.add(make_shared<sphere>(point3(-1, 0, -1), 0.5, material_left));
    //world.add(make_shared<sphere>(point3(-1, 0, -1), -0.4, material_left));
    hittable_list world = random_scene();

    // Camera
    point3 cameraPos(13, 2, 3);
    point3 lookat(0, 0, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;
    Camera camera(cameraPos, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

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
