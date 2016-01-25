//==============================================================================
// Copyright (c) 2007-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Saving images to JPG for PerfStudio
//==============================================================================

#ifndef SAVEIMAGE_H
#define SAVEIMAGE_H

#include <string>
using std::string;

enum OGL_IMAGE_FILE_FORMAT
{
    OGL_IFF_JPG,
    OGL_IFF_BMP,
    OGL_IFF_PNG,
    // Uncomment this when we can save a texture as DDS
    //OGL_IFF_DDS,
};

/// Converts an input string into a an image format enum.
/// \param strFormat input string format e.g. "PNG"
/// \return Image format enum
OGL_IMAGE_FILE_FORMAT GetOGLFileFormat(std::string strFormat);

/// Converts an RGB 8-bit per channel texture into a JPEG image
/// \param pDIB input pixel data
/// \param iWidth width of the texture
/// \param iHeight height of the texture
/// \param[out] ulSize will contain the number of byte in the output texture
/// \param[out] pData will contain the Jpeg image data
/// \return true if the data could be converted correctly; false otherwise
bool RGBtoJpeg(unsigned char* pDIB, int iWidth, int iHeight, UINT32* ulSize, unsigned char** pData);

/// Converts an RGBA 8-bit per channel texture into a BMP image
/// \param pImageData input pixel data
/// \param iWidth width of the texture
/// \param iHeight height of the texture
/// \param[out] pulSize will contain the number of byte in the output texture
/// \param[out] ppOutData will contain the BMP image data
/// \return true if the data could be converted correctly; false otherwise
bool RGBAtoBMP(unsigned char* pImageData, int iWidth, int iHeight, UINT32* pulSize, unsigned char** ppOutData);

/// Converts an RGBA 8-bit per channel texture into a PNG image
/// \param pImageData input pixel data
/// \param iWidth width of the texture
/// \param iHeight height of the texture
/// \param[out] pulSize will contain the number of byte in the output texture
/// \param[out] ppOutData will contain the BMP image data
/// \return true if the data could be converted correctly; false otherwise
bool RGBAtoPNG(unsigned char* pImageData, int iWidth, int iHeight, UINT32* pulSize, unsigned char** ppOutData);

#if defined (_WIN32)
#pragma comment( lib, "gdiplus.lib" )
#elif defined (_LINUX)
// Manually define the required bitmap data structures, and ensure they are packed
// to match the required sizes
typedef struct tagBITMAPFILEHEADER
{
    WORD    bfType;
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD      biSize;
    LONG       biWidth;
    LONG       biHeight;
    WORD       biPlanes;
    WORD       biBitCount;
    DWORD      biCompression;
    DWORD      biSizeImage;
    LONG       biXPelsPerMeter;
    LONG       biYPelsPerMeter;
    DWORD      biClrUsed;
    DWORD      biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
#endif // _WIN32

#endif // SAVEIMAGE_H
