#include <Windows.h>
#include "RTStbImage.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "../External/stb_image.h"


#define MAX_PATH_LEN 1024
std::string GetCurrentPath()
{
    std::string ret;
    char buf[MAX_PATH_LEN];
    GetCurrentDirectoryA(MAX_PATH_LEN, buf);

    char* p = strrchr(buf, '\\');
    if (strncmp(p, "\\Release", strlen(p)) == 0 || strncmp(p, "\\Debug", strlen(p)) == 0)
    {
        ret = std::string(buf) + "\\..\\..\\..\\Textures\\";
    }
    else
    {
        ret = std::string(buf) + "\\..\\Textures\\";
    }
    return ret;
}


RTStbImage::RTStbImage(const char* szFileName)
{
    auto strFileName = std::string(szFileName);
    auto imageDir = GetCurrentPath();
    //earthmap.jpg

    if (!imageDir.empty() && Load(imageDir + strFileName))
        return;
    else 
        std::cerr << "ERROR: Could not load image file '" << imageDir + strFileName << "'.\n";
}

RTStbImage::~RTStbImage()
{
    STBI_FREE(data);
}

bool RTStbImage::Load(const std::string szFileName)
{
    auto n = bPerPixel;
    data = stbi_load(szFileName.c_str(), &imageWidth, &imageHeight, &n, bPerPixel);
    bPerScanline = imageWidth * bPerPixel;
    return data != nullptr;
}

const unsigned char* RTStbImage::PixelData(int x, int y) const
{
    // Ñóºì
    static unsigned char magenta[] = { 255,0,255 };
    if (nullptr == data) return magenta;
    x = Clamp(x, 0, imageWidth);
    y = Clamp(y, 0, imageHeight);
    return data + y * bPerScanline + x * bPerPixel;;
}

int RTStbImage::Clamp(int x, int low, int high)
{
    if (x < low)return low;
    if (x < high)return x;
    return high - 1;
}
