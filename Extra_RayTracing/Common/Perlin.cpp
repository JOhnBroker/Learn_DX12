#include "Perlin.h"

Perlin::Perlin()
{
	ranVec = new vec3[siPointCount];
	for (int i = 0; i < siPointCount; ++i) 
	{
		ranVec[i] = unit_vector(vec3::random(-1, 1));
	}
	permX = PerlinGeneratePerm();
	permY = PerlinGeneratePerm();
	permZ = PerlinGeneratePerm();
}

Perlin::~Perlin()
{
	delete[] ranVec;
	delete[] permX;
	delete[] permY;
	delete[] permZ;
}

double Perlin::Noise(const point3& p) const
{
	double u = p.x() - floor(p.x());
	double v = p.y() - floor(p.y());
	double w = p.z() - floor(p.z());

	u = u * u * (3 - 2 * u);
	v = v * v * (3 - 2 * v);
	w = w * w * (3 - 2 * w);

	int i = static_cast<int>(floor(p.x()));
	int j = static_cast<int>(floor(p.y()));
	int k = static_cast<int>(floor(p.z()));

	vec3 c[2][2][2];

	for (int di = 0; di < 2; ++di)
		for (int dj = 0; dj < 2; ++dj)
			for (int dk = 0; dk < 2; ++dk)
				c[di][dj][dk] = ranVec[
					permX[(i + di) & 255] ^
					permY[(j + dj) & 255] ^
					permZ[(k + dk) & 255]];

	return PerlinInterp(c, u, v, w);
}

double Perlin::Turb(const point3& p, int depth) const
{
	auto accum = 0.0;
	auto temp_p = p;
	auto weight = 1.0;
	
	for (int i = 0; i < depth; ++i) 
	{
		accum += weight * Noise(temp_p);
		weight *= 0.5;
		temp_p *= 2;
	}

	return fabs(accum);
}

int* Perlin::PerlinGeneratePerm()
{
	auto p = new int[siPointCount];

	for (int i = 0; i < siPointCount; ++i) 
	{
		p[i] = i;
	}
	Permute(p, siPointCount);
	return p;
}

void Perlin::Permute(int* p, int n)
{
	for (int i = n - 1; i > 0; --i) 
	{
		int target = random_int(0, i);
		int tmp = p[i];
		p[i] = p[target];
		p[target] = tmp;
	}
}

double Perlin::TrilinearInterp(double c[2][2][2], double u, double v, double w)
{
	double accum = 0.0;

	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < 2; ++j)
			for (int k = 0; k < 2; ++k)
				accum += (i * u + (1 - i) * (1 - u)) *
						(j * v + (1 - j) * (1 - v)) *
						(k * w + (1 - k) * (1 - w)) *
						c[i][j][k];

	return accum;
}

double Perlin::PerlinInterp(vec3 c[2][2][2], double u, double v, double w)
{
	auto accum = 0.0;

	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < 2; ++j)
			for (int k = 0; k < 2; ++k) 
			{
				vec3 weight_v(u - i, v - j, w - k);
				accum += (i * u + (1 - i) * (1 - u)) *
					(j * v + (1 - j) * (1 - v)) *
					(k * w + (1 - k) * (1 - w)) *
					dot(c[i][j][k], weight_v);
			}

	return accum;
}
