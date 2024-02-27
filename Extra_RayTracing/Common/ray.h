#ifndef RAY_H
#define RAY_H
//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

#include "vec3.h"


class Ray {
  public:
    Ray() {}

    Ray(const point3& origin, const vec3& direction) 
        : orig(origin), dir(direction), t(0) {}
    Ray(const point3& origin, const vec3& direction, double time = 0.0) 
        : orig(origin), dir(direction), t(time) {}

    point3  GetOrigin() const  { return orig; }
    vec3    GetDirection() const { return dir; }
    double  GetTime()const { return t; }

    point3  At(double t) const { return orig + t * dir; }

  private:
    point3 orig;
    vec3 dir;
    double t;
};

#endif