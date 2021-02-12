// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"

#include "DxFontRenderData.h"

static constexpr float POINTS_PER_INCH = 72.0f;
static constexpr std::wstring_view FALLBACK_LOCALE = L"en-us";

using namespace Microsoft::Console::Render;

DxFontRenderData::DxFontRenderData(::Microsoft::WRL::ComPtr<IDWriteFactory1> dwriteFactory) noexcept :
    _dwriteFactory(dwriteFactory),
    _glyphCell{},
    _lineMetrics({}),
    _boxDrawingEffect{}
{
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextAnalyzer1> DxFontRenderData::Analyzer()
{
    if (!_dwriteTextAnalyzer)
    {
        Microsoft::WRL::ComPtr<IDWriteTextAnalyzer> analyzer;
        THROW_IF_FAILED(_dwriteFactory->CreateTextAnalyzer(&analyzer));
        THROW_IF_FAILED(analyzer.As(&_dwriteTextAnalyzer));
    }

    return _dwriteTextAnalyzer;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteFontFallback> DxFontRenderData::SystemFontFallback()
{
    if (!_systemFontFallback)
    {
        ::Microsoft::WRL::ComPtr<IDWriteFactory2> factory2;
        THROW_IF_FAILED(_dwriteFactory.As(&factory2));
        factory2->GetSystemFontFallback(&_systemFontFallback);
    }

    return _systemFontFallback;
}

[[nodiscard]] std::wstring DxFontRenderData::UserLocaleName()
{
    if (_userLocaleName.empty())
    {
        std::array<wchar_t, LOCALE_NAME_MAX_LENGTH> localeName;

        const auto returnCode = GetUserDefaultLocaleName(localeName.data(), gsl::narrow<int>(localeName.size()));
        if (returnCode)
        {
            _userLocaleName = { localeName.data() };
        }
        else
        {
            _userLocaleName = { FALLBACK_LOCALE.data(), FALLBACK_LOCALE.size() };
        }
    }

    return _userLocaleName;
}

[[nodiscard]] til::size DxFontRenderData::GlyphCell() noexcept
{
    return _glyphCell;
}

[[nodiscard]] DxFontRenderData::LineMetrics DxFontRenderData::GetLineMetrics() noexcept
{
    return _lineMetrics;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> DxFontRenderData::DefaultTextFormat()
{
    if (!_dwriteTextFormat)
    {
        std::wstring localeName = UserLocaleName();
        THROW_IF_FAILED(_defaultFontInfo.ToTextFormat(_dwriteFactory.Get(), _fontSize, localeName).As(&_dwriteTextFormat));
        THROW_IF_FAILED(_dwriteTextFormat->SetLineSpacing(_lineSpacing.method, _lineSpacing.height, _lineSpacing.baseline));
        THROW_IF_FAILED(_dwriteTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));
        THROW_IF_FAILED(_dwriteTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));

    }
    return _dwriteTextFormat;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteFontFace1> DxFontRenderData::DefaultFontFace()
{
    if (!_dwriteFontFace)
    {
        std::wstring fontLocaleName = UserLocaleName();
        // _ResolveFontFaceWithFallback overrides the last argument with the locale name of the font,
        // but we should use the system's locale to render the text.
        _dwriteFontFace = _defaultFontInfo.ResolveFontFaceWithFallback(_dwriteFactory.Get(), fontLocaleName);
    }

    return _dwriteFontFace;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IBoxDrawingEffect> DxFontRenderData::DefaultBoxDrawingEffect()
{
    if (!_boxDrawingEffect)
    {
        // Calculate and cache the box effect for the base font. Scale is 1.0f because the base font is exactly the scale we want already.
        THROW_IF_FAILED(s_CalculateBoxEffect(DefaultTextFormat().Get(), _glyphCell.width(), DefaultFontFace().Get(), 1.0f, &_boxDrawingEffect));
    }

    return _boxDrawingEffect;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> DxFontRenderData::ItalicTextFormat()
{
    if (!_dwriteTextFormatItalic)
    {
        std::wstring localeName = UserLocaleName();
        DxFontInfo fontInfoItalic = _defaultFontInfo;
        fontInfoItalic.SetStyle(DWRITE_FONT_STYLE_ITALIC);
        THROW_IF_FAILED(fontInfoItalic.ToTextFormat(_dwriteFactory.Get(), _fontSize, localeName).As(&_dwriteTextFormatItalic));
        THROW_IF_FAILED(_dwriteTextFormatItalic->SetLineSpacing(_lineSpacing.method, _lineSpacing.height, _lineSpacing.baseline));
        THROW_IF_FAILED(_dwriteTextFormatItalic->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));
        THROW_IF_FAILED(_dwriteTextFormatItalic->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
    }

    return _dwriteTextFormatItalic;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteFontFace1> DxFontRenderData::ItalicFontFace()
{
    if (!_dwriteFontFaceItalic)
    {
        DxFontInfo fontInfoItalic = _defaultFontInfo;
        fontInfoItalic.SetStyle(DWRITE_FONT_STYLE_ITALIC);
        std::wstring fontLocaleName = UserLocaleName();
        _dwriteFontFaceItalic = fontInfoItalic.ResolveFontFaceWithFallback(_dwriteFactory.Get(), fontLocaleName);
    }

    return _dwriteFontFaceItalic;
}

// Routine Description:
// - Updates the font used for drawing
// Arguments:
// - desired - Information specifying the font that is requested
// - actual - Filled with the nearest font actually chosen for drawing
// - dpi - The DPI of the screen
// Return Value:
// - S_OK or relevant DirectX error
[[nodiscard]] HRESULT DxFontRenderData::UpdateFont(const FontInfoDesired& desired, FontInfo& actual, const int dpi) noexcept
{
    try
    {
        _dwriteFontFace.Reset();
        _dwriteTextFormat.Reset();
        _dwriteFontFaceItalic.Reset();
        _dwriteTextFormatItalic.Reset();

        _defaultFontInfo = DxFontInfo(desired.GetFaceName(),
                        desired.GetWeight(),
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL);

        _BuildDefaultFontMetrics(desired, actual, dpi);

        // Create the font with the fractional pixel height size.
        // It should have an integer pixel width by our math above.
        // Then below, apply the line spacing to the format to position the floating point pixel height characters
        // into a cell that has an integer pixel height leaving some padding above/below as necessary to round them out.
    }
    CATCH_RETURN();

    return S_OK;
}

// Routine Description:
// - Calculates the box drawing scale/translate matrix values to fit a box glyph into the cell as perfectly as possible.
// Arguments:
// - format - Text format used to determine line spacing (height including ascent & descent) as calculated from the base font.
// - widthPixels - The pixel width of the available cell.
// - face - The font face that is currently being used, may differ from the base font from the layout.
// - fontScale -  if the given font face is going to be scaled versus the format, we need to know so we can compensate for that. pass 1.0f for no scaling.
// - effect - Receives the effect to apply to box drawing characters. If no effect is received, special treatment isn't required.
// Return Value:
// - S_OK, GSL/WIL errors, DirectWrite errors, or math errors.
[[nodiscard]] HRESULT STDMETHODCALLTYPE DxFontRenderData::s_CalculateBoxEffect(IDWriteTextFormat* format, size_t widthPixels, IDWriteFontFace1* face, float fontScale, IBoxDrawingEffect** effect) noexcept
try
{
    // Check for bad in parameters.
    RETURN_HR_IF(E_INVALIDARG, !format);
    RETURN_HR_IF(E_INVALIDARG, !face);

    // Check the out parameter and fill it up with null.
    RETURN_HR_IF(E_INVALIDARG, !effect);
    *effect = nullptr;

    // The format is based around the main font that was specified by the user.
    // We need to know its size as well as the final spacing that was calculated around
    // it when it was first selected to get an idea of how large the bounding box is.
    const auto fontSize = format->GetFontSize();

    DWRITE_LINE_SPACING_METHOD spacingMethod;
    float lineSpacing; // total height of the cells
    float baseline; // vertical position counted down from the top where the characters "sit"
    RETURN_IF_FAILED(format->GetLineSpacing(&spacingMethod, &lineSpacing, &baseline));

    const float ascentPixels = baseline;
    const float descentPixels = lineSpacing - baseline;

    // We need this for the designUnitsPerEm which will be required to move back and forth between
    // Design Units and Pixels. I'll elaborate below.
    DWRITE_FONT_METRICS1 fontMetrics;
    face->GetMetrics(&fontMetrics);

    // If we had font fallback occur, the size of the font given to us (IDWriteFontFace1) can be different
    // than the font size used for the original format (IDWriteTextFormat).
    const auto scaledFontSize = fontScale * fontSize;

    // This is Unicode FULL BLOCK U+2588.
    // We presume that FULL BLOCK should be filling its entire cell in all directions so it should provide a good basis
    // in knowing exactly where to touch every single edge.
    // We're also presuming that the other box/line drawing glyphs were authored in this font to perfectly inscribe
    // inside of FULL BLOCK, with the same left/top/right/bottom bearings so they would look great when drawn adjacent.
    const UINT32 blockCodepoint = L'\x2588';

    // Get the index of the block out of the font.
    UINT16 glyphIndex;
    RETURN_IF_FAILED(face->GetGlyphIndicesW(&blockCodepoint, 1, &glyphIndex));

    // If it was 0, it wasn't found in the font. We're going to try again with
    // Unicode BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL U+253C which should be touching
    // all the edges of the possible rectangle, much like a full block should.
    if (glyphIndex == 0)
    {
        const UINT32 alternateCp = L'\x253C';
        RETURN_IF_FAILED(face->GetGlyphIndicesW(&alternateCp, 1, &glyphIndex));
    }

    // If we still didn't find the glyph index, we haven't implemented any further logic to figure out the box dimensions.
    // So we're just going to leave successfully as is and apply no scaling factor. It might look not-right, but it won't
    // stop the rendering pipeline.
    RETURN_HR_IF(S_FALSE, glyphIndex == 0);

    // Get the metrics of the given glyph, which we're going to treat as the outline box in which all line/block drawing
    // glyphs will be inscribed within, perfectly touching each edge as to align when two cells meet.
    DWRITE_GLYPH_METRICS boxMetrics = { 0 };
    RETURN_IF_FAILED(face->GetDesignGlyphMetrics(&glyphIndex, 1, &boxMetrics));

    // NOTE: All metrics we receive from DWRITE are going to be in "design units" which are a somewhat agnostic
    //       way of describing proportions.
    //       Converting back and forth between real pixels and design units is possible using
    //       any font's specific fontSize and the designUnitsPerEm FONT_METRIC value.
    //
    // Here's what to know about the boxMetrics:
    //
    //
    //
    //   topLeft --> +--------------------------------+    ---
    //               |         ^                      |     |
    //               |         |  topSide             |     |
    //               |         |  Bearing             |     |
    //               |         v                      |     |
    //               |      +-----------------+       |     |
    //               |      |                 |       |     |
    //               |      |                 |       |     | a
    //               |      |                 |       |     | d
    //               |      |                 |       |     | v
    //               +<---->+                 |       |     | a
    //               |      |                 |       |     | n
    //               | left |                 |       |     | c
    //               | Side |                 |       |     | e
    //               | Bea- |                 |       |     | H
    //               | ring |                 | right |     | e
    //  vertical     |      |                 | Side  |     | i
    //  OriginY -->  x      |                 | Bea-  |     | g
    //               |      |                 | ring  |     | h
    //               |      |                 |       |     | t
    //               |      |                 +<----->+     |
    //               |      +-----------------+       |     |
    //               |                     ^          |     |
    //               |       bottomSide    |          |     |
    //               |          Bearing    |          |     |
    //               |                     v          |     |
    //               +--------------------------------+    ---
    //
    //
    //               |                                |
    //               +--------------------------------+
    //               |         advanceWidth           |
    //
    //
    // NOTE: The bearings can be negative, in which case it is specifying that the glyphs overhang the box
    // as defined by the advanceHeight/width.
    // See also: https://docs.microsoft.com/en-us/windows/win32/api/dwrite/ns-dwrite-dwrite_glyph_metrics

    // The scale is a multiplier and the translation is addition. So *1 and +0 will mean nothing happens.
    const float defaultBoxVerticalScaleFactor = 1.0f;
    float boxVerticalScaleFactor = defaultBoxVerticalScaleFactor;
    const float defaultBoxVerticalTranslation = 0.0f;
    float boxVerticalTranslation = defaultBoxVerticalTranslation;
    {
        // First, find the dimensions of the glyph representing our fully filled box.

        // Ascent is how far up from the baseline we'll draw.
        // verticalOriginY is the measure from the topLeft corner of the bounding box down to where
        // the glyph's version of the baseline is.
        // topSideBearing is how much "gap space" is left between that topLeft and where the glyph
        // starts drawing. Subtract the gap space to find how far is drawn upward from baseline.
        const auto boxAscentDesignUnits = boxMetrics.verticalOriginY - boxMetrics.topSideBearing;

        // Descent is how far down from the baseline we'll draw.
        // advanceHeight is the total height of the drawn bounding box.
        // verticalOriginY is how much was given to the ascent, so subtract that out.
        // What remains is then the descent value. Remove the
        // bottomSideBearing as the "gap space" on the bottom to find how far is drawn downward from baseline.
        const auto boxDescentDesignUnits = boxMetrics.advanceHeight - boxMetrics.verticalOriginY - boxMetrics.bottomSideBearing;

        // The height, then, of the entire box is just the sum of the ascent above the baseline and the descent below.
        const auto boxHeightDesignUnits = boxAscentDesignUnits + boxDescentDesignUnits;

        // Second, find the dimensions of the cell we're going to attempt to fit within.
        // We know about the exact ascent/descent units in pixels as calculated when we chose a font and
        // adjusted the ascent/descent for a nice perfect baseline and integer total height.
        // All we need to do is adapt it into Design Units so it meshes nicely with the Design Units above.
        // Use the formula: Pixels * Design Units Per Em / Font Size = Design Units
        const auto cellAscentDesignUnits = ascentPixels * fontMetrics.designUnitsPerEm / scaledFontSize;
        const auto cellDescentDesignUnits = descentPixels * fontMetrics.designUnitsPerEm / scaledFontSize;
        const auto cellHeightDesignUnits = cellAscentDesignUnits + cellDescentDesignUnits;

        // OK, now do a few checks. If the drawn box touches the top and bottom of the cell
        // and the box is overall tall enough, then we'll not bother adjusting.
        // We will presume the font author has set things as they wish them to be.
        const auto boxTouchesCellTop = boxAscentDesignUnits >= cellAscentDesignUnits;
        const auto boxTouchesCellBottom = boxDescentDesignUnits >= cellDescentDesignUnits;
        const auto boxIsTallEnoughForCell = boxHeightDesignUnits >= cellHeightDesignUnits;

        // If not...
        if (!(boxTouchesCellTop && boxTouchesCellBottom && boxIsTallEnoughForCell))
        {
            // Find a scaling factor that will make the total height drawn of this box
            // perfectly fit the same number of design units as the cell.
            // Since scale factor is a multiplier, it doesn't matter that this is design units.
            // The fraction between the two heights in pixels should be exactly the same
            // (which is what will matter when we go to actually render it... the pixels that is.)
            // Don't scale below 1.0. If it'd shrink, just center it at the prescribed scale.
            boxVerticalScaleFactor = std::max(cellHeightDesignUnits / boxHeightDesignUnits, 1.0f);

            // The box as scaled might be hanging over the top or bottom of the cell (or both).
            // We find out the amount of overhang/underhang on both the top and the bottom.
            const auto extraAscent = boxAscentDesignUnits * boxVerticalScaleFactor - cellAscentDesignUnits;
            const auto extraDescent = boxDescentDesignUnits * boxVerticalScaleFactor - cellDescentDesignUnits;

            // This took a bit of time and effort and it's difficult to put into words, but here goes.
            // We want the average of the two magnitudes to find out how much to "take" from one and "give"
            // to the other such that both are equal. We presume the glyphs are designed to be drawn
            // centered in their box vertically to look good.
            // The ordering around subtraction is required to ensure that the direction is correct with a negative
            // translation moving up (taking excess descent and adding to ascent) and positive is the opposite.
            const auto boxVerticalTranslationDesignUnits = (extraAscent - extraDescent) / 2;

            // The translation is just a raw movement of pixels up or down. Since we were working in Design Units,
            // we need to run the opposite algorithm shown above to go from Design Units to Pixels.
            boxVerticalTranslation = boxVerticalTranslationDesignUnits * scaledFontSize / fontMetrics.designUnitsPerEm;
        }
    }

    // The horizontal adjustments follow the exact same logic as the vertical ones.
    const float defaultBoxHorizontalScaleFactor = 1.0f;
    float boxHorizontalScaleFactor = defaultBoxHorizontalScaleFactor;
    const float defaultBoxHorizontalTranslation = 0.0f;
    float boxHorizontalTranslation = defaultBoxHorizontalTranslation;
    {
        // This is the only difference. We don't have a horizontalOriginX from the metrics.
        // However, https://docs.microsoft.com/en-us/windows/win32/api/dwrite/ns-dwrite-dwrite_glyph_metrics says
        // the X coordinate is specified by half the advanceWidth to the right of the horizontalOrigin.
        // So we'll use that as the "center" and apply it the role that verticalOriginY had above.

        const auto boxCenterDesignUnits = boxMetrics.advanceWidth / 2;
        const auto boxLeftDesignUnits = boxCenterDesignUnits - boxMetrics.leftSideBearing;
        const auto boxRightDesignUnits = boxMetrics.advanceWidth - boxMetrics.rightSideBearing - boxCenterDesignUnits;
        const auto boxWidthDesignUnits = boxLeftDesignUnits + boxRightDesignUnits;

        const auto cellWidthDesignUnits = widthPixels * fontMetrics.designUnitsPerEm / scaledFontSize;
        const auto cellLeftDesignUnits = cellWidthDesignUnits / 2;
        const auto cellRightDesignUnits = cellLeftDesignUnits;

        const auto boxTouchesCellLeft = boxLeftDesignUnits >= cellLeftDesignUnits;
        const auto boxTouchesCellRight = boxRightDesignUnits >= cellRightDesignUnits;
        const auto boxIsWideEnoughForCell = boxWidthDesignUnits >= cellWidthDesignUnits;

        if (!(boxTouchesCellLeft && boxTouchesCellRight && boxIsWideEnoughForCell))
        {
            boxHorizontalScaleFactor = std::max(cellWidthDesignUnits / boxWidthDesignUnits, 1.0f);
            const auto extraLeft = boxLeftDesignUnits * boxHorizontalScaleFactor - cellLeftDesignUnits;
            const auto extraRight = boxRightDesignUnits * boxHorizontalScaleFactor - cellRightDesignUnits;

            const auto boxHorizontalTranslationDesignUnits = (extraLeft - extraRight) / 2;

            boxHorizontalTranslation = boxHorizontalTranslationDesignUnits * scaledFontSize / fontMetrics.designUnitsPerEm;
        }
    }

    // If we set anything, make a drawing effect. Otherwise, there isn't one.
    if (defaultBoxVerticalScaleFactor != boxVerticalScaleFactor ||
        defaultBoxVerticalTranslation != boxVerticalTranslation ||
        defaultBoxHorizontalScaleFactor != boxHorizontalScaleFactor ||
        defaultBoxHorizontalTranslation != boxHorizontalTranslation)
    {
        // OK, make the object that will represent our effect, stuff the metrics into it, and return it.
        RETURN_IF_FAILED(WRL::MakeAndInitialize<BoxDrawingEffect>(effect, boxVerticalScaleFactor, boxVerticalTranslation, boxHorizontalScaleFactor, boxHorizontalTranslation));
    }

    return S_OK;
}
CATCH_RETURN()

void DxFontRenderData::_BuildDefaultFontMetrics(const FontInfoDesired& desired, FontInfo& actual, const int dpi)
{
    const auto face = DefaultFontFace();

    const UINT32 spaceCodePoint = L'M';
    UINT16 spaceGlyphIndex;
    THROW_IF_FAILED(face->GetGlyphIndicesW(&spaceCodePoint, 1, &spaceGlyphIndex));

    INT32 advanceInDesignUnits;
    THROW_IF_FAILED(face->GetDesignGlyphAdvances(1, &spaceGlyphIndex, &advanceInDesignUnits));

    DWRITE_GLYPH_METRICS spaceMetrics = { 0 };
    THROW_IF_FAILED(face->GetDesignGlyphMetrics(&spaceGlyphIndex, 1, &spaceMetrics));

    DWRITE_FONT_METRICS1 fontMetrics;
    face->GetMetrics(&fontMetrics);

    // The math here is actually:
    // Requested Size in Points * DPI scaling factor * Points to Pixels scaling factor.
    // - DPI = dots per inch
    // - PPI = points per inch or "points" as usually seen when choosing a font size
    // - The DPI scaling factor is the current monitor DPI divided by 96, the default DPI.
    // - The Points to Pixels factor is based on the typography definition of 72 points per inch.
    //    As such, converting requires taking the 96 pixel per inch default and dividing by the 72 points per inch
    //    to get a factor of 1 and 1/3.
    // This turns into something like:
    // - 12 ppi font * (96 dpi / 96 dpi) * (96 dpi / 72 points per inch) = 16 pixels tall font for 100% display (96 dpi is 100%)
    // - 12 ppi font * (144 dpi / 96 dpi) * (96 dpi / 72 points per inch) = 24 pixels tall font for 150% display (144 dpi is 150%)
    // - 12 ppi font * (192 dpi / 96 dpi) * (96 dpi / 72 points per inch) = 32 pixels tall font for 200% display (192 dpi is 200%)
    float heightDesired = static_cast<float>(desired.GetEngineSize().Y) * static_cast<float>(USER_DEFAULT_SCREEN_DPI) / POINTS_PER_INCH;

    // The advance is the number of pixels left-to-right (X dimension) for the given font.
    // We're finding a proportional factor here with the design units in "ems", not an actual pixel measurement.

    // Now we play trickery with the font size. Scale by the DPI to get the height we expect.
    heightDesired *= (static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI));

    const float widthAdvance = static_cast<float>(advanceInDesignUnits) / fontMetrics.designUnitsPerEm;

    // Use the real pixel height desired by the "em" factor for the width to get the number of pixels
    // we will need per character in width. This will almost certainly result in fractional X-dimension pixels.
    const float widthApprox = heightDesired * widthAdvance;

    // Since we can't deal with columns of the presentation grid being fractional pixels in width, round to the nearest whole pixel.
    const float widthExact = round(widthApprox);

    // Now reverse the "em" factor from above to turn the exact pixel width into a (probably) fractional
    // height in pixels of each character. It's easier for us to pad out height and align vertically
    // than it is horizontally.
    const auto fontSize = widthExact / widthAdvance;
    _fontSize = fontSize;

    // Now figure out the basic properties of the character height which include ascent and descent
    // for this specific font size.
    const float ascent = (fontSize * fontMetrics.ascent) / fontMetrics.designUnitsPerEm;
    const float descent = (fontSize * fontMetrics.descent) / fontMetrics.designUnitsPerEm;

    // Get the gap.
    const float gap = (fontSize * fontMetrics.lineGap) / fontMetrics.designUnitsPerEm;
    const float halfGap = gap / 2;

    // We're going to build a line spacing object here to track all of this data in our format.
    DWRITE_LINE_SPACING lineSpacing = {};
    lineSpacing.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;

    // We need to make sure the baseline falls on a round pixel (not a fractional pixel).
    // If the baseline is fractional, the text appears blurry, especially at small scales.
    // Since we also need to make sure the bounding box as a whole is round pixels
    // (because the entire console system maths in full cell units),
    // we're just going to ceiling up the ascent and descent to make a full pixel amount
    // and set the baseline to the full round pixel ascent value.
    //
    // For reference, for the letters "ag":
    // ...
    //          gggggg      bottom of previous line
    //
    // -----------------    <===========================================|
    //                         | topSideBearing       |  1/2 lineGap    |
    // aaaaaa   ggggggg     <-------------------------|-------------|   |
    //      a   g    g                                |             |   |
    //  aaaaa   ggggg                                 |<-ascent     |   |
    // a    a   g                                     |             |   |---- lineHeight
    // aaaaa a  gggggg      <----baseline, verticalOriginY----------|---|
    //          g     g                               |<-descent    |   |
    //          gggggg      <-------------------------|-------------|   |
    //                         | bottomSideBearing    | 1/2 lineGap     |
    // -----------------    <===========================================|
    //
    // aaaaaa   ggggggg     top of next line
    // ...
    //
    // Also note...
    // We're going to add half the line gap to the ascent and half the line gap to the descent
    // to ensure that the spacing is balanced vertically.
    // Generally speaking, the line gap is added to the ascent by DirectWrite itself for
    // horizontally drawn text which can place the baseline and glyphs "lower" in the drawing
    // box than would be desired for proper alignment of things like line and box characters
    // which will try to sit centered in the area and touch perfectly with their neighbors.

    const auto fullPixelAscent = ceil(ascent + halfGap);
    const auto fullPixelDescent = ceil(descent + halfGap);
    lineSpacing.height = fullPixelAscent + fullPixelDescent;
    lineSpacing.baseline = fullPixelAscent;

    // According to MSDN (https://docs.microsoft.com/en-us/windows/win32/api/dwrite_3/ne-dwrite_3-dwrite_font_line_gap_usage)
    // Setting "ENABLED" means we've included the line gapping in the spacing numbers given.
    lineSpacing.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_ENABLED;

    _lineSpacing = lineSpacing;

    // The scaled size needs to represent the pixel box that each character will fit within for the purposes
    // of hit testing math and other such multiplication/division.
    COORD coordSize = { 0 };
    coordSize.X = gsl::narrow<SHORT>(widthExact);
    coordSize.Y = gsl::narrow_cast<SHORT>(lineSpacing.height);

    // Unscaled is for the purposes of re-communicating this font back to the renderer again later.
    // As such, we need to give the same original size parameter back here without padding
    // or rounding or scaling manipulation.
    const COORD unscaled = desired.GetEngineSize();

    const COORD scaled = coordSize; 

    actual.SetFromEngine(_defaultFontInfo.GetFamilyName(),
                         desired.GetFamily(),
                         _defaultFontInfo.GetWeight(),
                         false,
                         scaled,
                         unscaled);

    // There is no font metric for the grid line width, so we use a small
    // multiple of the font size, which typically rounds to a pixel.
    _lineMetrics.gridlineWidth = std::round(fontSize * 0.025f);

    // All other line metrics are in design units, so to get a pixel value,
    // we scale by the font size divided by the design-units-per-em.
    const auto scale = fontSize / fontMetrics.designUnitsPerEm;
    _lineMetrics.underlineOffset = std::round(fontMetrics.underlinePosition * scale);
    _lineMetrics.underlineWidth = std::round(fontMetrics.underlineThickness * scale);
    _lineMetrics.strikethroughOffset = std::round(fontMetrics.strikethroughPosition * scale);
    _lineMetrics.strikethroughWidth = std::round(fontMetrics.strikethroughThickness * scale);

    // We always want the lines to be visible, so if a stroke width ends up
    // at zero after rounding, we need to make it at least 1 pixel.
    _lineMetrics.gridlineWidth = std::max(_lineMetrics.gridlineWidth, 1.0f);
    _lineMetrics.underlineWidth = std::max(_lineMetrics.underlineWidth, 1.0f);
    _lineMetrics.strikethroughWidth = std::max(_lineMetrics.strikethroughWidth, 1.0f);

    // Offsets are relative to the base line of the font, so we subtract
    // from the ascent to get an offset relative to the top of the cell.
    _lineMetrics.underlineOffset = fullPixelAscent - _lineMetrics.underlineOffset;
    _lineMetrics.strikethroughOffset = fullPixelAscent - _lineMetrics.strikethroughOffset;

    // For double underlines we need a second offset, just below the first,
    // but with a bit of a gap (about double the grid line width).
    _lineMetrics.underlineOffset2 = _lineMetrics.underlineOffset +
                                    _lineMetrics.underlineWidth +
                                    std::round(fontSize * 0.05f);

    // However, we don't want the underline to extend past the bottom of the
    // cell, so we clamp the offset to fit just inside.
    const auto maxUnderlineOffset = lineSpacing.height - _lineMetrics.underlineWidth;
    _lineMetrics.underlineOffset2 = std::min(_lineMetrics.underlineOffset2, maxUnderlineOffset);

    // But if the resulting gap isn't big enough even to register as a thicker
    // line, it's better to place the second line slightly above the first.
    if (_lineMetrics.underlineOffset2 < _lineMetrics.underlineOffset + _lineMetrics.gridlineWidth)
    {
        _lineMetrics.underlineOffset2 = _lineMetrics.underlineOffset - _lineMetrics.gridlineWidth;
    }

    // We also add half the stroke width to the offsets, since the line
    // coordinates designate the center of the line.
    _lineMetrics.underlineOffset += _lineMetrics.underlineWidth / 2.0f;
    _lineMetrics.underlineOffset2 += _lineMetrics.underlineWidth / 2.0f;
    _lineMetrics.strikethroughOffset += _lineMetrics.strikethroughWidth / 2.0f;

    _glyphCell = actual.GetSize();
}
