#ifndef ONB_H
#define ONB_H

#include "Common/vec3.h"

class ONB {
public:
	ONB(const vec3& n);
	
	const vec3& u() const { return axis[0]; }
	const vec3& v() const { return axis[1]; }
	const vec3& w() const { return axis[2]; }

	vec3 transform(const vec3& v) const;

private:
	vec3 axis[3];
};



#endif // !ONB_H
