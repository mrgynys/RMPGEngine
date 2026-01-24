#include "FontRasterizer.h"
#include <memory>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static bool EnsureGdiplusStarted()
{
    static bool started = false;
    static ULONG_PTR token = 0;
    if (started) return true;
    GdiplusStartupInput input;
    Status s = GdiplusStartup(&token, &input, nullptr);
    if (s == Ok)
    {
        started = true;
        return true;
    }
    return false;
}

HRESULT RasterizeTextToBGRA(
    const std::wstring& fontFilePath,
    const std::wstring& text,
    int fontPixels,
    std::vector<unsigned char>& outPixels,
    int& outWidth,
    int& outHeight,
    int padding)
{
    if (!EnsureGdiplusStarted())
        return E_FAIL;

    if (text.empty())
    {
        outPixels.clear();
        outWidth = outHeight = 0;
        return S_OK;
    }

    PrivateFontCollection pfc;
    Status s = pfc.AddFontFile(fontFilePath.c_str());
    if (s != Ok)
        return E_FAIL;

    int familyCount = pfc.GetFamilyCount();
    if (familyCount <= 0)
        return E_FAIL;

    std::unique_ptr<FontFamily[]> families(new FontFamily[familyCount]);
    int retrieved = 0;
    pfc.GetFamilies(familyCount, families.get(), &retrieved);
    if (retrieved <= 0)
        return E_FAIL;

    FontFamily& family = families[0];
    Font font(&family, static_cast<REAL>(fontPixels), FontStyleRegular, UnitPixel);

    Bitmap measureBmp(1, 1, PixelFormat32bppARGB);
    Graphics measureG(&measureBmp);
    measureG.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    RectF layoutRect;
    StringFormat fmt = StringFormat::GenericTypographic();

    RectF bound;
    Status st = measureG.MeasureString(text.c_str(), -1, &font, layoutRect, &bound);
    if (st != Ok)
    {
        bound.Width = static_cast<REAL>(text.length() * fontPixels);
        bound.Height = static_cast<REAL>(fontPixels * 1.2f);
    }

    int bmpW = static_cast<int>(ceil(bound.Width)) + padding * 2;
    int bmpH = static_cast<int>(ceil(bound.Height)) + padding * 2;
    if (bmpW <= 0) bmpW = 1;
    if (bmpH <= 0) bmpH = 1;

    Bitmap bmp(bmpW, bmpH, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(Color(0, 0, 0, 0));

    SolidBrush brush(Color(255, 255, 255, 255));

    RectF drawRect((REAL)padding, (REAL)padding, (REAL)(bmpW - padding * 2), (REAL)(bmpH - padding * 2));
    Status drawStatus = g.DrawString(text.c_str(), -1, &font, drawRect, &fmt, &brush);
    if (drawStatus != Ok)
    {
        StringFormat sf;
        sf.SetAlignment(StringAlignmentNear);
        sf.SetLineAlignment(StringAlignmentNear);
        drawStatus = g.DrawString(text.c_str(), -1, &font, drawRect, &sf, &brush);
        if (drawStatus != Ok)
            return E_FAIL;
    }

    Rect lockRect(0, 0, bmpW, bmpH);
    BitmapData bmpData;
    Status lockStatus = bmp.LockBits(&lockRect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    if (lockStatus != Ok)
        return E_FAIL;

    outWidth = bmpW;
    outHeight = bmpH;
    outPixels.resize(outWidth * outHeight * 4);
    const unsigned char* src = reinterpret_cast<const unsigned char*>(bmpData.Scan0);
    int srcPitch = bmpData.Stride;
    for (int y = 0; y < outHeight; ++y)
    {
        const unsigned char* row = src + y * srcPitch;
        unsigned char* dst = outPixels.data() + y * outWidth * 4;
        memcpy(dst, row, outWidth * 4);
    }

    bmp.UnlockBits(&bmpData);
    return S_OK;
}

HRESULT RasterizeTextRunsToBGRA(
    const std::vector<TextRun>& runs,
    std::vector<unsigned char>& outPixels,
    int& outWidth,
    int& outHeight,
    int padding)
{
    outPixels.clear();
    outWidth = outHeight = 0;
    if (!EnsureGdiplusStarted())
        return E_FAIL;
    if (runs.empty())
        return S_OK;

    struct RunMeasure
    {
        float width = 0.0f;
        float ascent = 0.0f;
        float descent = 0.0f;
        int pixelSize = 0;
        std::unique_ptr<PrivateFontCollection> pfc; // keep font collection alive
    };
    std::vector<RunMeasure> measures;
    measures.reserve(runs.size());

    Bitmap tmpBmp(1, 1, PixelFormat32bppARGB);
    Graphics measureG(&tmpBmp);
    measureG.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    float maxAscent = 0.0f, maxDescent = 0.0f;
    float totalWidth = 0.0f;

    for (const auto& r : runs)
    {
        auto pfc = std::make_unique<PrivateFontCollection>();
        if (pfc->AddFontFile(r.fontFile.c_str()) != Ok)
            return E_FAIL;

        int famCount = pfc->GetFamilyCount();
        if (famCount <= 0)
            return E_FAIL;

        std::unique_ptr<FontFamily[]> families(new FontFamily[famCount]);
        int retrieved = 0;
        pfc->GetFamilies(famCount, families.get(), &retrieved);
        if (retrieved <= 0)
            return E_FAIL;

        // use local family instance for measuring (no storing of FontFamily)
        FontFamily& familyRef = families[0];
        Font font(&familyRef, static_cast<REAL>(r.fontPixelSize), FontStyleRegular, UnitPixel);

        StringFormat fmt = StringFormat::GenericTypographic();
        fmt.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces);

        RectF layout;
        RectF bound;
        Status st = measureG.MeasureString(r.text.c_str(), -1, &font, layout, &bound);
        float w = (st == Ok) ? bound.Width : (r.text.length() * r.fontPixelSize * 0.6f);

        int emHeight = familyRef.GetEmHeight(FontStyleRegular);
        int cellAscent = familyRef.GetCellAscent(FontStyleRegular);
        int cellDescent = familyRef.GetCellDescent(FontStyleRegular);

        float ascentPx = (static_cast<float>(r.fontPixelSize) * cellAscent) / static_cast<float>(emHeight);
        float descentPx = (static_cast<float>(r.fontPixelSize) * cellDescent) / static_cast<float>(emHeight);

        RunMeasure rm;
        rm.width = w;
        rm.ascent = ascentPx;
        rm.descent = descentPx;
        rm.pixelSize = r.fontPixelSize;
        rm.pfc = std::move(pfc); // store pfc so font family remains available later

        measures.push_back(std::move(rm));

        if (ascentPx > maxAscent) maxAscent = ascentPx;
        if (descentPx > maxDescent) maxDescent = descentPx;
        totalWidth += w;
    }

    int bmpW = static_cast<int>(ceil(totalWidth)) + padding * 2;
    int bmpH = static_cast<int>(ceil(maxAscent + maxDescent)) + padding * 2;
    if (bmpW <= 0) bmpW = 1;
    if (bmpH <= 0) bmpH = 1;

    Bitmap bmp(bmpW, bmpH, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(Color(0, 0, 0, 0));

    float x = static_cast<float>(padding);
    for (size_t i = 0; i < runs.size(); ++i)
    {
        const auto& r = runs[i];
        const auto& m = measures[i];

        // recover the FontFamily from stored PrivateFontCollection
        FontFamily familyLocal;
        int retrieved = 0;
        // populate a single FontFamily instance from pfc
        m.pfc->GetFamilies(1, &familyLocal, &retrieved);
        if (retrieved <= 0)
            return E_FAIL;

        Font font(&familyLocal, static_cast<REAL>(r.fontPixelSize), FontStyleRegular, UnitPixel);

        float y = static_cast<float>(padding) + (maxAscent - m.ascent);

        BYTE a = static_cast<BYTE>((r.argbColor >> 24) & 0xFF);
        BYTE rr = static_cast<BYTE>((r.argbColor >> 16) & 0xFF);
        BYTE gg = static_cast<BYTE>((r.argbColor >> 8) & 0xFF);
        BYTE bb = static_cast<BYTE>(r.argbColor & 0xFF);
        SolidBrush brush(Color(a, rr, gg, bb));

        StringFormat fmt = StringFormat::GenericTypographic();
        fmt.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces);

        RectF layoutRect(x, y, m.width, static_cast<REAL>(bmpH - padding * 2));
        Status st = g.DrawString(r.text.c_str(), -1, &font, layoutRect, &fmt, &brush);
        x += m.width;
    }

    Rect lockRect(0, 0, bmpW, bmpH);
    BitmapData bmpData;
    Status lockStatus = bmp.LockBits(&lockRect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    if (lockStatus != Ok)
        return E_FAIL;
    outWidth = bmpW;
    outHeight = bmpH;
    outPixels.resize(outWidth * outHeight * 4);
    const unsigned char* src = reinterpret_cast<const unsigned char*>(bmpData.Scan0);
    int srcPitch = bmpData.Stride;
    for (int y = 0; y < outHeight; ++y)
    {
        const unsigned char* row = src + y * srcPitch;
        unsigned char* dst = outPixels.data() + y * outWidth * 4;
        memcpy(dst, row, outWidth * 4);
    }
    bmp.UnlockBits(&bmpData);
    return S_OK;
}