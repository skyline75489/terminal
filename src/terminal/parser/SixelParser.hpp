#pragma once

#include <string>
#include "../adapter/DispatchTypes.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    enum class SixelBackgroundOption
    {
        Set,
        Remain
    };

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

        size_t _aspectRatio;
        SixelBackgroundOption _backgroundOption;
        size_t _horizontalGridSize;
    };
}

