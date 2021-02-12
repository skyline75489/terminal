// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <dwrite.h>
#include <dwrite_1.h>
#include <dwrite_2.h>
#include <dwrite_3.h>

namespace Microsoft::Console::Render
{
    class DxFontInfo
    {
    public:
        DxFontInfo() noexcept;

        DxFontInfo(std::wstring_view familyName,
                   DWRITE_FONT_WEIGHT weight,
                   DWRITE_FONT_STYLE style,
                   DWRITE_FONT_STRETCH stretch) noexcept;

        DxFontInfo(std::wstring_view familyName,
                   unsigned int weight,
                   DWRITE_FONT_STYLE style,
                   DWRITE_FONT_STRETCH stretch) noexcept;

        DxFontInfo(const DxFontInfo& other);

        std::wstring GetFamilyName() const;
        void SetFamilyName(const std::wstring_view familyName) noexcept;

        DWRITE_FONT_WEIGHT GetWeight() const;
        void SetWeight(const DWRITE_FONT_WEIGHT weight) noexcept;

        DWRITE_FONT_STYLE GetStyle() const;
        void SetStyle(const DWRITE_FONT_STYLE style) noexcept;

        DWRITE_FONT_STRETCH GetStretch() const;
        void SetStretch(const DWRITE_FONT_STRETCH stretch) noexcept;

        void SetFromEngine(const std::wstring_view familyName,
                           const DWRITE_FONT_WEIGHT weight,
                           const DWRITE_FONT_STYLE style,
                           const DWRITE_FONT_STRETCH stretch);

        [[nodiscard]] ::Microsoft::WRL::ComPtr<IDWriteFontFace1> ResolveFontFaceWithFallback(IDWriteFactory1* dwriteFactory,
                                                                                             std::wstring& localeName);
        [[nodiscard]] ::Microsoft::WRL::ComPtr<IDWriteTextFormat> ToTextFormat(IDWriteFactory1* dwriteFactory, const float fontSize, const std::wstring_view localeName);


    private:
        [[nodiscard]] ::Microsoft::WRL::ComPtr<IDWriteFontFace1> _FindFontFace(IDWriteFactory1* dwriteFactory, std::wstring& localeName);

        [[nodiscard]] std::wstring _GetFontFamilyName(gsl::not_null<IDWriteFontFamily*> const fontFamily,
                                                      std::wstring& localeName);

        std::wstring _familyName;
        DWRITE_FONT_WEIGHT _weight;
        DWRITE_FONT_STYLE _style;
        DWRITE_FONT_STRETCH _stretch;
    };
}
