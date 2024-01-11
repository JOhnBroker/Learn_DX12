#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

class Camera 
{
public:
	Camera() 
	{
		auto aspect_ratio = 16.0 / 9.0;
		auto viewport_height = 2.0;
		auto viewport_width = aspect_ratio * viewport_height;
		auto focal_length = 1.0;

		m_Origin = point3(0, 0, 0);
		m_Horizontal = vec3(viewport_width, 0, 0);
		m_Vertical = vec3(0, viewport_height, 0);
		m_LowerLeftCorner = m_Origin - m_Horizontal / 2 - m_Vertical / 2 - vec3(0, 0, focal_length);
	}

	ray GetRay(double u, double v) 
	{
		return ray(m_Origin, m_LowerLeftCorner + u * m_Horizontal + v * m_Vertical - m_Origin);
	}

private:
	point3 m_Origin;
	point3 m_LowerLeftCorner;
	vec3 m_Horizontal;
	vec3 m_Vertical;
};


#endif // !CAMERA_H
