#include "precomp.h"

#include "SixelParser.hpp"

using namespace Microsoft::Console::VirtualTerminal;

constexpr uint8_t defaultAspectRatio = 2;

SixelParser::SixelParser(const gsl::span<const size_t> parameters, std::wstring_view data) :
    _aspectRatio(defaultAspectRatio),
    _backgroundOption(SixelBackgroundOption::Set),
    _horizontalGridSize(0)
{
    _PrapareParemeters(parameters);
}

void SixelParser::_PrapareParemeters(const gsl::span<const size_t> parameters)
{
    if (parameters.empty())
    {
        return;
    }

    uint8_t p1 = defaultAspectRatio;

    if (parameters.size() >= 1)
    {
        switch (parameters[0])
        {
        case 0:
        case 1:
            p1 = 2;
            break;
        case 2:
            p1 = 5;
            break;
        case 3:
        case 4:
            p1 = 3;
            break;
        case 5:
        case 6:
            p1 = 2;
            break;
        case 7:
        case 8:
        case 9:
            p1 = 1;
            break;
        default:
            p1 = defaultAspectRatio;
            break;
        }
    }

    _aspectRatio = p1;

    if (parameters.size() >= 2)
    {
        switch (parameters[1])
        {
        case 0:
        case 2:
            _backgroundOption = SixelBackgroundOption::Set;
            break;
        case 1:
            _backgroundOption = SixelBackgroundOption::Remain;
        default:
            _backgroundOption = SixelBackgroundOption::Set;
            break;
        }
    }

    if (parameters.size() >= 3)
    {
        _horizontalGridSize = parameters[2];
    }
}
