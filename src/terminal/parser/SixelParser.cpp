#include "precomp.h"

#include "SixelParser.hpp"

using namespace Microsoft::Console::VirtualTerminal;

constexpr uint8_t MAX_PARAMETER_VALUE = 32767ui8;
constexpr uint8_t SIXEL_MAX_PALETTE = 256ui8;


static til::color sixelDefaultColorTable[] = {
    til::color::from_xrgb(0, 0, 0), /*  0 Black    */
    til::color::from_xrgb(20, 20, 80), /*  1 Blue     */
    til::color::from_xrgb(80, 13, 13), /*  2 Red      */
    til::color::from_xrgb(20, 80, 20), /*  3 Green    */
    til::color::from_xrgb(80, 20, 80), /*  4 Magenta  */
    til::color::from_xrgb(20, 80, 80), /*  5 Cyan     */
    til::color::from_xrgb(80, 80, 20), /*  6 Yellow   */
    til::color::from_xrgb(53, 53, 53), /*  7 Gray 50% */
    til::color::from_xrgb(26, 26, 26), /*  8 Gray 25% */
    til::color::from_xrgb(33, 33, 60), /*  9 Blue*    */
    til::color::from_xrgb(60, 26, 26), /* 10 Red*     */
    til::color::from_xrgb(33, 60, 33), /* 11 Green*   */
    til::color::from_xrgb(60, 33, 60), /* 12 Magenta* */
    til::color::from_xrgb(33, 60, 60), /* 13 Cyan*    */
    til::color::from_xrgb(60, 60, 33), /* 14 Yellow*  */
    til::color::from_xrgb(80, 80, 80), /* 15 Gray 75% */
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

// Routine Description:
// - Determines if a character is a valid number character, 0-9.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
static constexpr bool _isNumericParamValue(const wchar_t wch) noexcept
{
    return wch >= L'0' && wch <= L'9'; // 0x30 - 0x39
}

SixelParser::SixelParser(std::wstring_view data) :
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
    _palette({}),
    _data({})
{
    _InitPalette();
    _Parse(data);
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
    _palette({}),
    _data({})
{
    _PrepareParemeters(parameters);
    _InitPalette();
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

void SixelParser::_InitPalette()
{
    uint8_t n = 0;

    /* palette initialization */
    for (; n < 16; n++)
    {
        _palette.emplace_back(sixelDefaultColorTable[n]);
    }

    /* colors 16-231 are a 6x6x6 color cube */
    for (uint8_t r = 0; r < 6; r++)
    {
        for (uint8_t g = 0; g < 6; g++)
        {
            for (uint8_t b = 0; b < 6; b++)
            {
                _palette.emplace_back(til::color(r * 51, g * 51, b * 51));
            }
        }
    }

    /* colors 232-255 are a grayscale ramp, intentionally leaving out */
    for (uint8_t i = 0; i < 24; i++)
    {
        _palette.emplace_back(til::color(i * 11, i * 11, i * 11));
    }

    for (; n < SIXEL_MAX_PALETTE; n++)
    {
        _palette.emplace_back(til::color(255, 255, 255));
    }
}

void SixelParser::_Parse(std::wstring_view data)
{
    // Init buffer
    _Resize(1, 1);

    auto it = data.begin();
    const auto end = data.end();
    while (it != end)
    {
        const wchar_t wch = *it;

        switch (wch)
        {
        case SixelControlCodes::GraphicsRepeatIntroducer:
        {
            auto params = _AccumulateParameters(++it, end);
            _repeatCount = params.at(0);
            continue;
        }
        case SixelControlCodes::ColorIntroducer:
        {
            _colorIndex = 0;
            auto params = _AccumulateParameters(++it, end);
            _colorIndex = params.at(0);

            // Custom palette
            if (params.size() > 4)
            {
                const auto pu = params.at(1);
                const auto px = static_cast<uint16_t>(params.at(2));
                const auto py = static_cast<uint8_t>(params.at(3));
                const auto pz = static_cast<uint8_t>(params.at(4));

                if (pu == 1)
                {
                    _palette.at(_colorIndex) = til::color::from_hsl(
                        std::min<uint16_t>(px, 360ui16),
                        std::min<uint8_t>(py, 100ui8),
                        std::min<uint8_t>(pz, 100ui8),
                        255);
                }

                if (pu == 2)
                {
                    _palette.at(_colorIndex) = til::color::from_xrgb(
                        std::min<uint8_t>(static_cast<uint8_t>(px), 100ui8),
                        std::min<uint8_t>(py, 100ui8),
                        std::min<uint8_t>(pz, 100ui8));
                }
            }
            continue;
        }
        case SixelControlCodes::GraphicsCarriageReturn:
        {
            _posX = 0;
            ++it;
            continue;
        }
        case SixelControlCodes::GraphicsNewLine:
        {
            _posX = 0;
            _posY += 6;
            ++it;
            continue;
        }
        case SixelControlCodes::RasterAttributes:
        {
            auto params = _AccumulateParameters(++it, end);
            _attrPad = params.at(0);
            continue;
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
                            _data.at(_posY + i).at(_posX) = _palette.at(_colorIndex);
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
                                std::fill(start, cend, _palette.at(_colorIndex));
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
            }

            it++;
        }
    }

    _width = _height;
}

std::vector<size_t> SixelParser::_AccumulateParameters(std::wstring_view::const_iterator& it, std::wstring_view::const_iterator end) noexcept
{
    std::vector<size_t> result;
    size_t value = 0;

    while (it != end)
    {
        const wchar_t wch = *it;

        if (_isParameterDelimiter(wch))
        {
            result.emplace_back(value);
            value = 0;
            it++;
            continue;
        }

        if (!_isNumericParamValue(wch))
        {
            result.emplace_back(value);
            return result;
        }

        const auto digit = wch - L'0';

        value = value * 10 + digit;

        // Values larger than the maximum should be mapped to the largest supported value.
        if (value > MAX_PARAMETER_VALUE)
        {
            value = MAX_PARAMETER_VALUE;
        }

        it++;
    }

    return result;
}

void SixelParser::_Resize(size_t width, size_t height)
{
    if (height >= _height)
    {
        _data.resize(height);
        _height = height;
    }

    if (width >= _width)
    {
        for (auto &row : _data)
        {
            row.resize(width);
        }
        _width = width;
    }
}
