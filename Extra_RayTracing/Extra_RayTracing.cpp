#include "Common/common.h"

#include "camera.h"
#include "ConstantMedium.h"
#include "hittable_list.h"
#include "BVH.h"
#include "material.h"
#include "Quad.h"
#include "sphere.h"
#include "Common/Texture.h"

#include <memory>

/*
void RandomSpheresScene() 
{
    hittable_list world;

    auto ground_maerial = make_shared<Lambertian>(color(0.5, 0.5, 0.5));
    auto checker = make_shared<CheckerTexture>(0.32, color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<Lambertian>(checker)));

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
                    auto cen2 = center + vec3(0, random_double(0, 0.5f), 0);
                    world.add(make_shared<sphere>(center, cen2, r, material));
                }
                else if (choose_mat < 0.95) 
                {
                    auto albedo = color::random() * color::random();
                    auto fuzz = random_double(0, 0.5);
                    material = make_shared<Metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, r, material));
                }
                else 
                {
                    // glass
                    material = make_shared<Dielectric>(1.5);
                    world.add(make_shared<sphere>(center, r, material));
                }
            }
        }
    }

    auto material1 = make_shared<Dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<Lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<Metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    world = hittable_list(make_shared<BVHNode>(world));

    // Camera
    Camera camera;

    // Image
    camera.aspect_ratio = 16.0 / 9.0;
    camera.image_width = 400;
    camera.samples_per_pixel = 100;
    camera.max_depth = 50;
    camera.background = color(0.7, 0.8, 1.0);

    camera.vfov = 20;
    camera.lookfrom = point3(13, 2, 3);
    camera.lookat = point3(0, 0, 0);
    camera.vup = vec3(0, 1, 0);

    camera.defocus_angle = 0.02;
    camera.focus_dist = 10.0;

    // Render
    camera.Render(world);
}

void TwoSpheresScene()
{
    hittable_list world;

    //auto checker = make_shared<CheckerTexture>(0.8, color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    //world.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<Lambertian>(checker)));
    //world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<Lambertian>(checker)));

    auto noise = make_shared<NoiseTexture>(2);
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<Lambertian>(noise)));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<Lambertian>(noise)));

    // Camera
    Camera camera;

    // Image
    camera.aspect_ratio = 16.0 / 9.0;
    camera.image_width = 400;
    camera.samples_per_pixel = 100;
    camera.max_depth = 50;
    camera.background = color(0.7, 0.8, 1.0);

    camera.vfov = 20;
    camera.lookfrom = point3(13, 2, 3);
    camera.lookat = point3(0, 0, 0);
    camera.vup = vec3(0, 1, 0);

    camera.defocus_angle = 0;

    // Render
    camera.Render(world);
}

void EarthScene() 
{
    auto earthTexture = make_shared<ImageTexture>("earthmap.jpg");
    auto earthSurface = make_shared<Lambertian>(earthTexture);
    auto earth = make_shared<sphere>(point3(0, 0, 0), 2, earthSurface);

    // Camera
    Camera camera;

    // Image
    camera.aspect_ratio = 16.0 / 9.0;
    camera.image_width = 400;
    camera.samples_per_pixel = 100;
    camera.max_depth = 50;
    camera.background = color(0.7, 0.8, 1.0);

    camera.vfov = 20;
    camera.lookfrom = point3(12, 0, 0);
    camera.lookat = point3(0, 0, 0);
    camera.vup = vec3(0, 1, 0);

    camera.defocus_angle = 0;

    // Render
    camera.Render(hittable_list(earth));

}

void Quads()
{
    hittable_list world;

    // Material
    auto left_red       = make_shared<Lambertian>(color(1.0, 0.2, 0.2));
    auto back_green     = make_shared<Lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue     = make_shared<Lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange   = make_shared<Lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal     = make_shared<Lambertian>(color(0.2, 0.8, 0.8));

    // Quads
    world.add(make_shared<Quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), left_red));
    world.add(make_shared<Quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<Quad>(point3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<Quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<Quad>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4), lower_teal));

    Camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color(0.7, 0.8, 1.0);

    cam.vfov = 80;
    cam.lookfrom = point3(0, 0, 9);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.Render(world);
}

void SimpleLight() 
{
    hittable_list world;

    auto pertext = make_shared<NoiseTexture>(2);
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<Lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<Lambertian>(pertext)));

    auto diffLight = make_shared<DiffuseLight>(color(4, 4, 4));
    world.add(make_shared<sphere>(point3(0, 7, 0), 2, diffLight));
    world.add(make_shared<Quad>(point3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0), diffLight));

    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 20;
    cam.lookfrom = point3(26, 3, 6);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.Render(world);
}

void CornellBox() {
    hittable_list world;

    auto red = make_shared<Lambertian>(color(.65, .05, .05));
    auto white = make_shared<Lambertian>(color(.73, .73, .73));
    auto green = make_shared<Lambertian>(color(.12, .45, .15));
    auto light = make_shared<DiffuseLight>(color(7, 7, 7));

    world.add(make_shared<Quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
    world.add(make_shared<Quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
    world.add(make_shared<Quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light));
    world.add(make_shared<Quad>(point3(0, 555, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    world.add(make_shared<Quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    world.add(make_shared<Quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

    shared_ptr<Hittable> box1 = Box(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<RotateY>(box1, 15);
    box1 = make_shared<Translate>(box1, vec3(265, 0, 295));

    shared_ptr<Hittable> box2 = Box(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<RotateY>(box2, -18);
    box2 = make_shared<Translate>(box2, vec3(130, 0, 65));

    world.add(make_shared<ConstantMedium>(box1, 0.01, color(0, 0, 0)));
    world.add(make_shared<ConstantMedium>(box2, 0.01, color(1, 1, 1)));

    Camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 600;
    cam.samples_per_pixel = 20;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.Render(world);
}
*/

void CornellBoxMonteCarlo() {
    hittable_list world;

    auto red = make_shared<Lambertian>(color(.65, .05, .05));
    auto white = make_shared<Lambertian>(color(.73, .73, .73));
    auto green = make_shared<Lambertian>(color(.12, .45, .15));
    auto light = make_shared<DiffuseLight>(color(15, 15, 15));

    world.add(make_shared<Quad>(point3(555, 0, 0), vec3(0, 0, 555), vec3(0, 555, 0), green));
    world.add(make_shared<Quad>(point3(0, 0, 555), vec3(0, 0, -555), vec3(0, 555, 0), red));
    world.add(make_shared<Quad>(point3(0, 555, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    world.add(make_shared<Quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 0, -555), white));
    world.add(make_shared<Quad>(point3(555, 0, 555), vec3(-555, 0, 0), vec3(0, 555, 0), white));

    world.add(make_shared<Quad>(point3(213, 554, 227), vec3(130, 0, 0), vec3(0, 0, 105), light));

    shared_ptr<Hittable> box1 = Box(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<RotateY>(box1, 15);
    box1 = make_shared<Translate>(box1, vec3(265, 0, 295));

    auto glass = make_shared<Dielectric>(1.5);
    /*shared_ptr<Hittable> box2 = Box(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<RotateY>(box2, -18);
    box2 = make_shared<Translate>(box2, vec3(130, 0, 65));*/

    world.add(box1);
    world.add(std::make_shared<sphere>(point3(190, 90, 190), 90, glass));
    /*world.add(make_shared<ConstantMedium>(box1, 0.01, color(0, 0, 0)));
    world.add(make_shared<ConstantMedium>(box2, 0.01, color(1, 1, 1)));*/

    auto emptyMaterial = shared_ptr<Material>();
    Quad lights(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), emptyMaterial);

    Camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 600;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.Render(world, lights);
}

//void FinalScene(int image_width, int samples_per_pixel, int max_depth) {
//    hittable_list boxes1;
//    auto ground = make_shared<Lambertian>(color(0.48, 0.83, 0.53));
//
//    int boxes_per_side = 20;
//    for (int i = 0; i < boxes_per_side; i++) {
//        for (int j = 0; j < boxes_per_side; j++) {
//            auto w = 100.0;
//            auto x0 = -1000.0 + i * w;
//            auto z0 = -1000.0 + j * w;
//            auto y0 = 0.0;
//            auto x1 = x0 + w;
//            auto y1 = random_double(1, 101);
//            auto z1 = z0 + w;
//
//            boxes1.add(Box(point3(x0, y0, z0), point3(x1, y1, z1), ground));
//        }
//    }
//
//    hittable_list world;
//
//    world.add(make_shared<BVHNode>(boxes1));
//
//    auto light = make_shared<DiffuseLight>(color(7, 7, 7));
//    world.add(make_shared<Quad>(point3(123, 554, 147), vec3(300, 0, 0), vec3(0, 0, 265), light));
//
//    auto center1 = point3(400, 400, 200);
//    auto center2 = center1 + vec3(30, 0, 0);
//    auto sphere_material = make_shared<Lambertian>(color(0.7, 0.3, 0.1));
//    world.add(make_shared<sphere>(center1, center2, 50, sphere_material));
//
//    world.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<Dielectric>(1.5)));
//    world.add(make_shared<sphere>(
//        point3(0, 150, 145), 50, make_shared<Metal>(color(0.8, 0.8, 0.9), 1.0)
//    ));
//
//    auto boundary = make_shared<sphere>(point3(360, 150, 145), 70, make_shared<Dielectric>(1.5));
//    world.add(boundary);
//    world.add(make_shared<ConstantMedium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
//    boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<Dielectric>(1.5));
//    world.add(make_shared<ConstantMedium>(boundary, .0001, color(1, 1, 1)));
//
//    auto emat = make_shared<Lambertian>(make_shared<ImageTexture>("earthmap.jpg"));
//    world.add(make_shared<sphere>(point3(400, 200, 400), 100, emat));
//    auto pertext = make_shared<NoiseTexture>(0.2);
//    world.add(make_shared<sphere>(point3(220, 280, 300), 80, make_shared<Lambertian>(pertext)));
//
//    hittable_list boxes2;
//    auto white = make_shared<Lambertian>(color(.73, .73, .73));
//    int ns = 1000;
//    for (int j = 0; j < ns; j++) {
//        boxes2.add(make_shared<sphere>(point3::random(0, 165), 10, white));
//    }
//
//    world.add(make_shared<Translate>(
//        make_shared<RotateY>(
//            make_shared<BVHNode>(boxes2), 15),
//        vec3(-100, 270, 395)
//    )
//    );
//
//    Camera cam;
//
//    cam.aspect_ratio = 1.0;
//    cam.image_width = image_width;
//    cam.samples_per_pixel = samples_per_pixel;
//    cam.max_depth = max_depth;
//    cam.background = color(0, 0, 0);
//
//    cam.vfov = 40;
//    cam.lookfrom = point3(478, 278, -600);
//    cam.lookat = point3(278, 278, 0);
//    cam.vup = vec3(0, 1, 0);
//
//    cam.defocus_angle = 0;
//
//    cam.Render(world);
//}


int main()
{
    // World
    switch (8)
    {
    /*case 1:
        RandomSpheresScene();
        break;
    case 2:
        TwoSpheresScene();
        break;
    case 3:
        EarthScene();
        break;
    case 4:
        Quads();
        break;
    case 5:
        SimpleLight();
        break;
    case 6:
        CornellBox();
        break;*/
    case 8:
        CornellBoxMonteCarlo();
        break;
    /*case 7:
        FinalScene(400, 250, 4);
        break;*/
    }

    return 0;
}

