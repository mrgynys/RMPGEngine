#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

struct TextRun
{
    std::wstring text;
    std::wstring fontFile;
    int fontPixelSize;
    uint32_t argbColor;
};

HRESULT RasterizeTextToBGRA(
    const std::wstring& fontFilePath,
    const std::wstring& text,
    int fontPixels,
    std::vector<unsigned char>& outPixels,
    int& outWidth,
    int& outHeight,
    int padding = 2);

HRESULT RasterizeTextRunsToBGRA(
    const std::vector<TextRun>& runs,
    std::vector<unsigned char>& outPixels,
    int& outWidth,
    int& outHeight,
    int padding = 2);