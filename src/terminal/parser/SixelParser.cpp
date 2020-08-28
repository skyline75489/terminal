#include "precomp.h"

#include "SixelParser.hpp"

using namespace Microsoft::Console::VirtualTerminal;

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
    _attrPad(1),
    _attrPan(2),
    _repeatCount(1),
    _colorIndex(0),
    _posX(0),
    _posY(0),
    _maxX(0),
    _maxY(0),
    _width(0),
    _height(0),
    _data({})
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

    if (parameters.size() >= 1)
    {
        switch (parameters[0])
        {
        case 0:
        case 1:
            _attrPad = 2;
            break;
        case 2:
            _attrPad = 5;
            break;
        case 3:
        case 4:
            _attrPad = 4;
            break;
        case 5:
        case 6:
            _attrPad = 3;
            break;
        case 7:
        case 8:
            _attrPad = 2;
            break;
        case 9:
            _attrPad = 1;
            break;
        default:
            _attrPad = 2;
            break;
        }
    }

    size_t pn3 = 0;

    if (parameters.size() >= 3)
    {
        pn3 = parameters[2];
        if (pn3 == 0)
        {
            pn3 = 10;
        }

        _attrPan = _attrPan * pn3 / 10;
        _attrPad = _attrPad * pn3 / 10;

        if (_attrPan <= 0)
        {
            _attrPan = 1;
        }
        if (_attrPad <= 0)
        {
            _attrPad = 1;
        }
    }
}

void SixelParser::_Parse(std::wstring_view data)
{
    auto it = data.begin();
    const auto end = data.end();
    while (it != end)
    {
        const wchar_t wch = *it;

        switch (wch)
        {
        case SixelControlCodes::GraphicsRepeatIntroducer:
        {
            _AccumulateParameter(it++, end, _repeatCount);
            break;
        }
        case SixelControlCodes::ColorIntroducer:
        {
            _AccumulateParameter(it++, end, _colorIndex);
            break;
        }
        default:
        {
            break;
        }
        }

        if (_isDataStringCharacter(wch))
        {
            auto sx = _width;
            while (sx < _posX + _repeatCount)
            {
                sx *= 2;
            }

            auto sy = _height;
            while (sy < _posY + 6)
            {
                sy *= 2;
            }

            if (sx > _width || sy > _height)
            {
                _Resize(sx, sy);
            }

            const auto bits = wch - L'?';

            if (bits == 0)
            {
                _posX += _repeatCount;
            }
            else
            {
                auto sixel_vertical_mask = 0x01;
                if (_repeatCount <= 1)
                {
                    for (auto i = 0; i < 6; i++)
                    {
                        if ((bits & sixel_vertical_mask) != 0)
                        {
                            _data.at(_posY + i).at(_posX) = (char)_colorIndex;
                            _maxX = std::max(_maxX, _posX);
                            _maxY = std::max(_maxY, _posY + 1);
                        }
                        sixel_vertical_mask <<= 1;
                    }

                    _posX += 1;
                }
                else
                {
                    /* context->repeat_count > 1 */
                    for (auto i = 0; i < 6; i++)
                    {
                        if ((bits & sixel_vertical_mask) != 0)
                        {
                            auto c = sixel_vertical_mask << 1;
                            auto n = 1;
                            for (; (i + n) < 6; n++)
                            {
                                if ((bits & c) == 0)
                                {
                                    break;
                                }
                                c <<= 1;
                            }
                            for (auto y = _posY + i; y < _posY + i + n; ++y)
                            {
                                const auto start = std::next(_data.at(y).begin(), _posX);
                                const auto cend = start + _repeatCount;
                                std::fill(start, cend, (char)_colorIndex);
                            }
                            _maxX = std::max(_maxX, _posX + _repeatCount - 1);
                            _maxY = std::max(_maxY, _posY + i + n - 1);
                            i += (n - 1);
                            sixel_vertical_mask <<= (n - 1);
                        }
                        sixel_vertical_mask <<= 1;
                    }
                    _posX += _repeatCount;
                }

                it++;
            }
        }
    }
}

void SixelParser::_AccumulateParameter(std::wstring_view::const_iterator it, std::wstring_view::const_iterator end, size_t& value) noexcept
{
    while (it != end)
    {
        const wchar_t wch = *it;

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

void SixelParser::_Resize(size_t width, size_t height)
{
    if (height > _height)
    {
        _data.resize(height);
    }

    if (width > _width)
    {
        for (auto row : _data)
        {
            row.resize(width);
        }
    }
}
