#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

#include "color.h"
#include "hittable.h"
#include "material.h"

#include <iostream>


class Camera {
public:
    void Render(const hittable& world);

public:
    double aspect_ratio      = 1.0;  // Ratio of image width over height
    int    image_width       = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth         = 10;   // Maximum number of Ray bounces into scene

    double vfov     = 90;              // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,-1);  // Point camera is looking from
    point3 lookat   = point3(0,0,0);   // Point camera is looking at
    vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction

    double defocus_angle = 0;  // Variation angle of Rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

private:
    void Initialize();

    Ray GetRay(int i, int j) const;

    vec3 PixelSampleSquare() const;

    vec3 PixelSampleDisk(double radius) const;

    point3 DefocusDiskSample() const;

    color RayColor(const Ray& r, int depth, const hittable& world) const;

private:
    int    image_height;    // Rendered image height
    point3 center;          // Camera center
    point3 pixel00_loc;     // Location of pixel 0, 0
    vec3   pixel_delta_u;   // Offset to pixel to the right
    vec3   pixel_delta_v;   // Offset to pixel below
    vec3   u, v, w;         // Camera frame basis vectors
    vec3   defocus_disk_u;  // Defocus disk horizontal radius
    vec3   defocus_disk_v;  // Defocus disk vertical radius
};


#endif
