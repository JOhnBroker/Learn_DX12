#ifndef RT_STB_IMAGE_H
#define RT_STB_IMAGE_H

#ifdef _MSC_VER
	#pragma warning (push,0)
#endif // !_MSC_VER

#include <cstdlib>
#include <iostream>

class RTStbImage
{
public:
	RTStbImage() :data(nullptr) {}
	RTStbImage(const char* szFileName);
	~RTStbImage();

	bool Load(const std::string szFileName);
	int Width()const { return imageWidth; }
	int Height()const { return imageHeight; }

	const unsigned char* PixelData(int x, int y)const;

private:
	const int bPerPixel = 3;
	unsigned char* data = nullptr;
	int imageWidth = 0, imageHeight = 0;
	int bPerScanline = 0;

	static int Clamp(int x, int low, int high);
};

// Restore MSVC compiler warnings
#ifdef _MSC_VER
	#pragma warning (pop)
#endif

#endif // !RT_STB_IMAGE_H