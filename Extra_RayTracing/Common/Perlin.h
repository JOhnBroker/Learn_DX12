#ifndef PERLIN_H
#define PERLIN_H

#include "common.h"

class Perlin
{
public:
	Perlin();
	~Perlin();
	double Noise(const point3& p)const;
	double Turb(const point3& p, int depth = 7)const;
private:
	static const int siPointCount = 256;
	vec3* ranVec;
	int* permX;
	int* permY;
	int* permZ;

	static int* PerlinGeneratePerm();
	static void Permute(int* p, int n);
	static double TrilinearInterp(double c[2][2][2], double u, double v, double w);
	static double PerlinInterp(vec3 c[2][2][2], double u, double v, double w);
};

#endif // !PERLIN_H
