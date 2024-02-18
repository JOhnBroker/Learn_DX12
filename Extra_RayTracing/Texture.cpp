#include "Texture.h"

color CheckerTexture::Value(double u, double v, const point3& p) const
{
	auto xInteger = static_cast<int>(std::floor(invScale * p.x()));
	auto yInteger = static_cast<int>(std::floor(invScale * p.y()));
	auto zInteger = static_cast<int>(std::floor(invScale * p.z()));

	bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

	return isEven ? even->Value(u, v, p) : odd->Value(u, v, p);
}
