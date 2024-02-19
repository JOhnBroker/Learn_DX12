#include "Texture.h"

color CheckerTexture::Value(double u, double v, const point3& p) const
{
	auto xInteger = static_cast<int>(std::floor(invScale * p.x()));
	auto yInteger = static_cast<int>(std::floor(invScale * p.y()));
	auto zInteger = static_cast<int>(std::floor(invScale * p.z()));

	bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

	return isEven ? even->Value(u, v, p) : odd->Value(u, v, p);
}

color ImageTexture::Value(double u, double v, const point3& p) const
{
	if (image.Height() <= 0)return color(0, 1, 1);
	
	u = interval(0, 1).clamp(u);
	v = 1.0 - interval(0, 1).clamp(v);

	auto i = static_cast<int>(u * image.Width());
	auto j = static_cast<int>(v * image.Height());
	auto pixel = image.PixelData(i, j);

	auto colorScale = 1.0 / 255.0;
	return color(colorScale * pixel[0], colorScale * pixel[1], colorScale * pixel[2]);
}
