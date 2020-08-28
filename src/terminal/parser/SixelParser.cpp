#include "precomp.h"

#include "SixelParser.hpp"

using namespace Microsoft::Console::VirtualTerminal;

constexpr uint8_t defaultAspectRatio = 2;
constexpr size_t MAX_PARAMETER_VALUE = 32767;

constexpr til::color sixelDefaultColorTable[] = {
    til::color(0,  0,  0),   /*  0 Black    */
    til::color(20, 20, 80),  /*  1 Blue     */
    til::color(80, 13, 13),  /*  2 Red      */
    til::color(20, 80, 20),  /*  3 Green    */
    til::color(80, 20, 80),  /*  4 Magenta  */
    til::color(20, 80, 80),  /*  5 Cyan     */
    til::color(80, 80, 20),  /*  6 Yellow   */
    til::color(53, 53, 53),  /*  7 Gray 50% */
    til::color(26, 26, 26),  /*  8 Gray 25% */
    til::color(33, 33, 60),  /*  9 Blue*    */
    til::color(60, 26, 26),  /* 10 Red*     */
    til::color(33, 60, 33),  /* 11 Green*   */
    til::color(60, 33, 60),  /* 12 Magenta* */
    til::color(33, 60, 60),  /* 13 Cyan*    */
    til::color(60, 60, 33),  /* 14 Yellow*  */
    til::color(80, 80, 80),  /* 15 Gray 75% */
};

// Routine Description:
// - Determines if a character is a delimiter between two parameters in a "control sequence".
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
static constexpr bool _isParameterDelimiter(const wchar_t wch) noexcept
{
    return wch == L';'; // 0x3B
}

// Routine Description:
// - Determines if a character is a sixel data string character.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
static constexpr bool _isDataStringCharacter(const wchar_t wch) noexcept
{
    return wch >= L'?' && wch <= L'~'; // 0x3F - 0x7E
}

SixelParser::SixelParser(const gsl::span<const size_t> parameters, std::wstring_view data) :
    _aspectRatio(defaultAspectRatio),
    _backgroundOption(SixelBackgroundOption::Set),
    _horizontalGridSize(0)
{
    _PrepareParemeters(parameters);
    _Parse(data);
}

void SixelParser::_PrepareParemeters(const gsl::span<const size_t> parameters)
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
            break;
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

void SixelParser::_Parse(std::wstring_view data)
{
    auto it = data.begin();
    auto end = data.end();
    while (it != end)
    {
        wchar_t wch = *it;
        size_t repeatCount = 1;

        switch (wch)
        {
        case SixelControlCodes::GraphicsRepeatIntroducer:
        {
            _AccumulateParameter(it++, end, repeatCount);
        }
        }

        if (_isDataStringCharacter(wch))
        {
            it++;
        }
    }
}

void SixelParser::_AccumulateParameter(std::wstring_view::const_iterator it, std::wstring_view::const_iterator end, size_t& value) noexcept
{
    while (it != end)
    {
        wchar_t wch = *it;

        if (_isDataStringCharacter(wch) || _isParameterDelimiter(wch))
        {
            return;
        }

        const size_t digit = wch - L'0';

        value = value * 10 + digit;

        // Values larger than the maximum should be mapped to the largest supported value.
        if (value > MAX_PARAMETER_VALUE)
        {
            value = MAX_PARAMETER_VALUE;
        }
    }
}
