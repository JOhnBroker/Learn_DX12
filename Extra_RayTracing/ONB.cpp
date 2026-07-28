#include "ONB.h"

ONB::ONB(const vec3& n) {
	axis[2] = unit_vector(n);
	// Suppose we have any vector a
	// that is of nonzero length and nonparallel with n
	vec3 a = (std::fabs(axis[2].x()) > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0));
	axis[1] = unit_vector(cross(axis[2], a));
	axis[0] = cross(axis[2], axis[1]);
}

vec3 ONB::transform(const vec3& v) const {
	return (v[0] * axis[0]) + (v[1] * axis[1]) + (v[2] * axis[2]);
}