#pragma once

#include <string>

namespace Microsoft::Console::VirtualTerminal
{
    enum SixelBackgroundOption
    {
        Set,
        Remain
    };

    class SixelParser
    {
    public:
        SixelParser(const gsl::span<const size_t> parameters, std::wstring_view data);

    private:

        void _PrapareParemeters(const gsl::span<const size_t> parameters);

        uint8_t _aspectRatio;
        SixelBackgroundOption _backgroundOption;
        uint8_t _horizontalGridSize;
    };
}

