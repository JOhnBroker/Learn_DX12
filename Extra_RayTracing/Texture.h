#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"
#include "color.h"


class Texture
{
public:
	virtual ~Texture() = default;
	virtual color Value(double u, double v, const point3& p)const = 0;
};

class SolidColor :public Texture 
{
public:
	SolidColor(color c) :colorValue(c) {}
	SolidColor(double r, double g, double b) :SolidColor(color(r, g, b)) {}

	color Value(double u, double v, const point3& p) const override 
	{
		return colorValue;
	}

private:
	color colorValue;
};

// 格子图
class CheckerTexture :public Texture 
{
public:
	CheckerTexture(double scale, shared_ptr<Texture> _even, shared_ptr<Texture> _odd)
		:invScale(1.0 / scale), even(_even), odd(_odd) {}
	CheckerTexture(double scale, color c1, color c2)
		:invScale(1.0 / scale), even(make_shared<SolidColor>(c1)), odd(make_shared<SolidColor>(c2)) {}

	color Value(double u, double v, const point3& p) const override;
private:
	double invScale;
	shared_ptr<Texture> even;
	shared_ptr<Texture> odd;
};


#endif // !TEXTURE_H
