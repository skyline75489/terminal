#pragma once

#include <string>
#include "../adapter/DispatchTypes.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    enum SixelControlCodes : uint64_t
    {
        DECGRI_GraphicsRepeatIntroducer = VTID("!"),
        DECGRA_SetRasterAttributes = VTID("\""),
        DECGCI_GraphicsColorIntroducer = VTID("#"),
        DECGCR_GraphicsCarriageReturn = VTID("$"),
        DECGNL_GraphicsNewLine = VTID("-")
    };

    enum class SixelStates
    {
        DataString,
        RepeatIntroducer,
        RasterAttributes,
        ColorIntroducer,
    };

    class SixelParser
    {
    public:
        SixelParser(std::wstring_view data);
        SixelParser(const gsl::span<const size_t> parameters, std::wstring_view data);

        std::vector<std::vector<til::color>>& GetBitmapData() noexcept;

    private:
        void _PrepareParameters(const gsl::span<const size_t> parameters) noexcept;
        void _Parse(std::wstring_view data);
        void _InitPalette();
        void _AccumulateTo(const wchar_t wch, size_t& value) noexcept;
        void _Resize(size_t width, size_t height);

        void ProcessCharacter(const wchar_t wch);

        void _ActionControlCharacter(const wchar_t wch) noexcept;
        void _ActionParam(const wchar_t wch);
        ;
        void _ActionIgnore() noexcept;
        void _ActionDataString(const wchar_t wch);
        void _ActionRepeatIntroducer();
        void _ActionRasterAttribute() noexcept;
        void _ActionColorIntroducer();

        void _EnterDataString() noexcept;
        void _EnterRepeatIntroducer() noexcept;
        void _EnterRasterAttributes() noexcept;
        void _EnterColorIntroducer() noexcept;

        void _EventDataString(const wchar_t wch);
        void _EventRepeatIntroducer(const wchar_t wch);
        void _EventRasterAttributes(const wchar_t wch);
        void _EventColorIntroducer(const wchar_t wch);

        size_t _attrPixalDenominator;
        size_t _attrPixelNumerator;
        size_t _attrHorizontalSize;
        size_t _attrVerticalSize;

        size_t _repeatCount;
        size_t _colorIndex;

        size_t _posX;
        size_t _posY;

        size_t _maxX;
        size_t _maxY;

        size_t _width;
        size_t _height;

        SixelStates _state;
        std::vector<size_t> _parameters;

        std::vector<til::color> _palette;
        std::vector<std::vector<til::color>> _data;
    };
}
