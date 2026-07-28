#include "Quad.h"

Quad::Quad(const point3& q, const vec3& u, const vec3& v, shared_ptr<Material> mat)
	: mQ(q), mU(u), mV(v), mMat(mat)
{
	auto n = cross(mU, mV);
	mNormal = unit_vector(n);
	mD = dot(mNormal, mQ);
	mW = n / dot(n, n);
	mArea = n.length();

	SetBoundingBox();
}

bool Quad::Hit(const Ray& r, interval ray_t, HitRecord& rec) const
{
	// denominator
	auto denom = dot(mNormal, r.GetDirection());

	if (fabs(denom) < 1e-8) {
		return false;
	}

	auto t = (mD - dot(mNormal, r.GetOrigin())) / denom;
	if (!ray_t.contains(t)) {
		return false;
	}

	auto intersection = r.At(t);
	vec3 planarHitptVector = intersection - mQ;
	auto alpha = dot(mW, cross(planarHitptVector, mV));
	auto beta = dot(mW, cross(mU, planarHitptVector));

	if (!isInterior(alpha, beta, rec)) {
		return false;
	}	

	rec.t = t;
	rec.p = intersection;
	rec.mat = mMat;
	rec.set_face_normal(r, mNormal);

	return true;
}

bool Quad::isInterior(double a, double b, HitRecord& rec) const {
	interval unitInterval = interval(0, 1);

	// Given the hit point in plane coordinates, return false if it is outside the
	// primitive, otherwise set the hit record UV coordinates and return true.
	if (!unitInterval.contains(a) || !unitInterval.contains(b)) {
		return false;
	}
	rec.u = a;
	rec.v = b;
	return true;
}

double Quad::PDFValue(const point3& origin, const vec3& direction) const
{
	HitRecord rec;
	if (!this->Hit(Ray(origin, direction, 0), interval(0.001, infinity), rec))
		return 0;
	auto distanceSquared = rec.t * rec.t * direction.length_squared();
	auto cosine = std::fabs(dot(direction, rec.normal) / direction.length());
	return distanceSquared / (cosine * mArea);
}

vec3 Quad::Random(const point3& origin) const
{
	auto p = mQ + (random_double() * mU) + (random_double() * mV);
	return p - origin;
}
