#pragma once

#include <string>
#include "../adapter/DispatchTypes.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    enum SixelControlCodes : uint64_t
    {
        GraphicsRepeatIntroducer = VTID("!"),
        RasterAttributes = VTID("\""),
        ColorIntroducer = VTID("#"),
        GraphicsCarriageReturn = VTID("$"),
        GraphicsNewLine = VTID("-")
    };

    class SixelParser
    {
    public:
        SixelParser(const gsl::span<const size_t> parameters, std::wstring_view data);

    private:

        void _PrepareParemeters(const gsl::span<const size_t> parameters);
        void _Parse(std::wstring_view data);
        void _AccumulateParameter(std::wstring_view::const_iterator it, std::wstring_view::const_iterator end, size_t& value) noexcept;
        void _Resize(size_t width, size_t height);

        size_t _attrPad;
        size_t _attrPan;

        size_t _repeatCount;
        size_t _colorIndex;

        size_t _posX;
        size_t _posY;

        size_t _maxX;
        size_t _maxY;

        size_t _width;
        size_t _height;

        std::vector<std::vector<char>> _data;
    };
}

