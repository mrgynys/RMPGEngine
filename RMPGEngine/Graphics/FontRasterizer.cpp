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

struct TextFragment
{
    std::wstring text;
    int fontPixelSize;
    std::wstring fontFile;
    UINT32 argbColor;
    std::unique_ptr<PrivateFontCollection> pfc;
    bool endsWithLineBreak;
};

std::vector<TextFragment> SplitRunIntoFragments(const TextRun& run)
{
    std::vector<TextFragment> fragments;
    std::wstring current;
    for (size_t i = 0; i < run.text.size(); ++i)
    {
        if (run.text[i] == L'\n')
        {
            if (!current.empty())
                fragments.push_back({ current, run.fontPixelSize, run.fontFile, run.argbColor, nullptr, true });
            else
                fragments.push_back({ L"", run.fontPixelSize, run.fontFile, run.argbColor, nullptr, true });
            current.clear();
        }
        else
        {
            current += run.text[i];
        }
    }

    if (!current.empty() || fragments.empty())
        fragments.push_back({ current, run.fontPixelSize, run.fontFile, run.argbColor, nullptr, false });
    else if (!fragments.empty())
        fragments.back().endsWithLineBreak = false;

    return fragments;
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

    std::vector<TextFragment> allFragments;
    for (const auto& run : runs)
    {
        auto frags = SplitRunIntoFragments(run);
        for (auto& frag : frags)
        {
            auto pfc = std::make_unique<PrivateFontCollection>();
            if (pfc->AddFontFile(frag.fontFile.c_str()) != Ok)
                return E_FAIL;
            frag.pfc = std::move(pfc);
        }
        allFragments.insert(allFragments.end(), std::move_iterator(frags.begin()), std::move_iterator(frags.end()));
    }

    struct Line
    {
        std::vector<TextFragment*> fragments;
        float totalWidth = 0.0f;
        float maxAscent = 0.0f;
        float maxDescent = 0.0f;
    };
    std::vector<Line> lines;
    Line currentLine;

    Bitmap tmpBmp(1, 1, PixelFormat32bppARGB);
    Graphics measureG(&tmpBmp);
    measureG.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    for (auto& frag : allFragments)
    {
        FontFamily family;
        int retrieved = 0;
        frag.pfc->GetFamilies(1, &family, &retrieved);
        if (retrieved <= 0)
            return E_FAIL;
        Font font(&family, static_cast<REAL>(frag.fontPixelSize), FontStyleRegular, UnitPixel);

        RectF layout, bound;
        StringFormat fmt = StringFormat::GenericTypographic();
        fmt.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces);
        Status st = measureG.MeasureString(frag.text.c_str(), -1, &font, layout, &bound);
        float width = (st == Ok) ? bound.Width : (frag.text.length() * frag.fontPixelSize * 0.6f);
        frag.text = frag.text;
        int emHeight = family.GetEmHeight(FontStyleRegular);
        int cellAscent = family.GetCellAscent(FontStyleRegular);
        int cellDescent = family.GetCellDescent(FontStyleRegular);
        float ascentPx = (static_cast<float>(frag.fontPixelSize) * cellAscent) / static_cast<float>(emHeight);
        float descentPx = (static_cast<float>(frag.fontPixelSize) * cellDescent) / static_cast<float>(emHeight);

        currentLine.fragments.push_back(&frag);
        currentLine.totalWidth += width;
        if (ascentPx > currentLine.maxAscent) currentLine.maxAscent = ascentPx;
        if (descentPx > currentLine.maxDescent) currentLine.maxDescent = descentPx;

        if (frag.endsWithLineBreak)
        {
            lines.push_back(std::move(currentLine));
            currentLine = Line();
        }
    }

    if (!currentLine.fragments.empty() || lines.empty())
        lines.push_back(std::move(currentLine));

    float maxLineWidth = 0.0f;
    float totalHeight = 0.0f;
    std::vector<float> lineHeights;
    for (const auto& line : lines)
    {
        if (line.totalWidth > maxLineWidth) maxLineWidth = line.totalWidth;
        float lineHeight = line.maxAscent + line.maxDescent;
        lineHeights.push_back(lineHeight);
        totalHeight += lineHeight;
    }
    int bmpW = static_cast<int>(ceil(maxLineWidth)) + padding * 2;
    int bmpH = static_cast<int>(ceil(totalHeight)) + padding * 2;
    if (bmpW <= 0) bmpW = 1;
    if (bmpH <= 0) bmpH = 1;

    Bitmap bmp(bmpW, bmpH, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(Color(0, 0, 0, 0));

    float yOffset = static_cast<float>(padding);
    for (size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx)
    {
        const auto& line = lines[lineIdx];
        float x = static_cast<float>(padding);
        float baselineY = yOffset + line.maxAscent;

        for (auto* frag : line.fragments)
        {
            FontFamily family;
            int retrieved = 0;
            frag->pfc->GetFamilies(1, &family, &retrieved);
            if (retrieved <= 0)
                return E_FAIL;
            Font font(&family, static_cast<REAL>(frag->fontPixelSize), FontStyleRegular, UnitPixel);

            RectF layout, bound;
            StringFormat fmt = StringFormat::GenericTypographic();
            fmt.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces);
            g.MeasureString(frag->text.c_str(), -1, &font, layout, &bound);
            float width = bound.Width;

            int emHeight = family.GetEmHeight(FontStyleRegular);
            int cellAscent = family.GetCellAscent(FontStyleRegular);
            int cellDescent = family.GetCellDescent(FontStyleRegular);
            float ascentPx = (static_cast<float>(frag->fontPixelSize) * cellAscent) / static_cast<float>(emHeight);
            float y = baselineY - ascentPx;

            BYTE a = static_cast<BYTE>((frag->argbColor >> 24) & 0xFF);
            BYTE rr = static_cast<BYTE>((frag->argbColor >> 16) & 0xFF);
            BYTE gg = static_cast<BYTE>((frag->argbColor >> 8) & 0xFF);
            BYTE bb = static_cast<BYTE>(frag->argbColor & 0xFF);
            SolidBrush brush(Color(a, rr, gg, bb));

            RectF layoutRect(x, y, width, static_cast<REAL>(bmpH - padding * 2));
            g.DrawString(frag->text.c_str(), -1, &font, layoutRect, &fmt, &brush);

            x += width;
        }
        yOffset += line.maxAscent + line.maxDescent;
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
