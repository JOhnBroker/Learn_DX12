#ifndef PDF_H
#define PDF_H

#include "ONB.h"
#include "hittable_list.h"

class PDF
{
public:
	virtual ~PDF() {}
	virtual double Value(const vec3& direction) const = 0;
	virtual vec3 Generate() const = 0;
};

class SpherePDF :public PDF {
public:
	SpherePDF(){}
	double Value(const vec3& direction) const override {
		return 1 / (4 * pi);
	}
	vec3 Generate() const override{
		return random_unit_vector();
	}
};


class CosinePDF :public PDF {
public:
	CosinePDF(const vec3& w) :uvw(w) {}
	double Value(const vec3& direction) const override {
		auto cosine_theta = dot(unit_vector(direction), uvw.w());
		return std::fmax(0, cosine_theta / pi);
	}
	vec3 Generate() const override { 
		return uvw.transform(random_cosine_direction());
	}
private:
	ONB uvw;
};

class HittablePDF :public PDF {
public:
	HittablePDF(const Hittable& objects, const point3& origin)
		: objects(objects), origin(origin) {}
	double Value(const vec3& direction) const override {
		return objects.PDFValue(origin, direction);
	}
	vec3 Generate() const override {
		return objects.Random(origin);
	}

private:
	const Hittable& objects;
	point3 origin;
};

class MixturePDF :public PDF {
public:
	MixturePDF(shared_ptr<PDF> p0, shared_ptr<PDF> p1) {
		mP[0] = p0;
		mP[1] = p1;
	}
	double Value(const vec3& direction) const override {
		return 0.5 * mP[0]->Value(direction) + 0.5 * mP[1]->Value(direction);
	}
	vec3 Generate() const override {
		if (random_double() < 0.5)
			return mP[0]->Generate();
		else
			return mP[1]->Generate();
	}

private:
	shared_ptr<PDF> mP[2];
};

#endif // !PDF_H
