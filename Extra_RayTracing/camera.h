#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

class Camera 
{
public:
	Camera(
		point3 cameraPos, point3 lookat, vec3 vup,
		double vfov, double aspect_ratio,
		double aperture, double focus_dist)
	{
		auto theta = degrees_to_radians(vfov);
		auto tan_halftheta = tan(theta / 2);
		auto viewport_height = 2.0 * tan_halftheta;
		auto viewport_width = aspect_ratio * viewport_height;

		m_W = unit_vector(cameraPos - lookat);
		m_U = unit_vector(cross(vup, m_W));
		m_V = cross(m_W, m_U);

		m_Origin = cameraPos;
		m_Horizontal = focus_dist * viewport_width * m_U;
		m_Vertical = focus_dist * viewport_height * m_V;
		m_LowerLeftCorner = m_Origin - m_Horizontal / 2 - m_Vertical / 2 - focus_dist * m_W;

		m_LensRadius = aperture / 2;
	}

	ray GetRay(double u, double v) 
	{
		vec3 rd = m_LensRadius * random_in_unit_disk();
		vec3 offset = m_U * rd.x() + m_V * rd.y();

		return ray(m_Origin + offset, m_LowerLeftCorner + u * m_Horizontal + v * m_Vertical - m_Origin - offset);
	}

private:
	point3 m_Origin;
	point3 m_LowerLeftCorner;
	vec3 m_Horizontal;
	vec3 m_Vertical;
	vec3 m_U, m_V, m_W;
	double m_LensRadius;
};


#endif // !CAMERA_H
