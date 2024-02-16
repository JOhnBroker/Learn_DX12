#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"
#include "color.h"
#include "hittable.h"
#include "material.h"

#include <iostream>

class Camera 
{
public:
	void Render(const hittable& world);

public:
	double aspect_ratio = 1.0;
	int image_width = 100;
	int samples_per_pixel = 10;
	int max_depth = 10;

	double vfov = 90;
	point3 lookfrome = point3(0, 0, -1);
	point3 lookat = point3(0, 0, 0);
	vec3 vup = vec3(0, 1, 0);

	double defocus_angle = 0;
	double focus_dist = 10;

private:
	void Initialize();
	ray GetRay(int i, int j);
	vec3 PixelSampleSquare() const;
	vec3 PixelSampleDisk(double radius)const;
	point3 DefocusDiskSample()const;
	color RayColor(const ray& r, int depth, const hittable& world)const;

private:
	int image_height;
	point3 m_Origin;
	point3 m_LowerLeftCorner;
	vec3 m_PixelDeltaU;
	vec3 m_PixelDeltaV;
	vec3 m_U, m_V, m_W;
	vec3 m_DefocusDiskU; //Defocus disk horizontal radius
	vec3 m_DefocusDiskV; //Defocus disk vertical radius
};


#endif // !CAMERA_H
