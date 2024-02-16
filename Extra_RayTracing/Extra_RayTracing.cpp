#include "common.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"


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
    // World
    hittable_list world = random_scene();

    // Camera
    Camera camera;

    // Image
    camera.aspect_ratio = 16.0 / 9.0;
    camera.image_width = 1200;
    camera.samples_per_pixel = 10;
    camera.max_depth = 20;

    camera.vfov = 20;
    camera.lookfrome= point3(13, 2, 3);
    camera.lookat = point3(0, 0, 0);
    camera.vup = vec3(0, 1, 0);

    camera.defocus_angle = 0.6;
    camera.focus_dist = 10.0;

    // Render
    camera.Render(world);

    return 0;
}
