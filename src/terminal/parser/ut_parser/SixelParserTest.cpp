// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "WexTestClass.h"
#include "../../inc/consoletaeftemplates.hpp"

#include "SixelParser.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class SixelParserTest;
        };
    };
};

using namespace Microsoft::Console::VirtualTerminal;

static auto SIXEL_BLUE = til::color::from_xrgb(20, 20, 80);
static auto SIXEL_RED = til::color::from_xrgb(80, 13, 13);
static auto SIXEL_GREEN = til::color::from_xrgb(20, 80, 20);

static auto SIXEL_BACKGROUND = til::color();

constexpr uint8_t sixel_mask(const wchar_t sixelChar)
{
    return (sixelChar - L'?') & 0b111111;
}

class SixelParserTest final
{
    TEST_CLASS(SixelParserTest);

    TEST_METHOD(TestParseSingleSixel)
    {
        SixelParser parser(L"#1N");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 1);
        verify_sixel(result, 0, 0, L'N', SIXEL_BLUE);
    }

    TEST_METHOD(TestParseSingleRowOneColor)
    {
        SixelParser parser(L"#1NNNN");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 4);

        verify_sixel_row(result, 0, 0, 4, L'N', SIXEL_BLUE);
    }

    TEST_METHOD(TestParseSingleRowMultiColorSameSixel)
    {
        SixelParser parser(L"#1NNNN#2NNNN");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 8);

        verify_sixel_row(result, 0, 0, 4, L'N', SIXEL_BLUE);
        verify_sixel_row(result, 4, 0, 4, L'N', SIXEL_RED);
    }

    TEST_METHOD(TestParseSingleRowMultiColorDifferentSixel)
    {
        SixelParser parser(L"#1NNNN#2oooo");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 8);

        verify_sixel_row(result, 0, 0, 4, L'N', SIXEL_BLUE);
        verify_sixel_row(result, 4, 0, 4, L'o', SIXEL_RED);
    }

    TEST_METHOD(TestParseCarriageReturn)
    {
        SixelParser parser(L"#1NNNN#2NNNN#3NNNN$oooo#1oooo#2oooo");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 16);

        verify_sixel_row(result, 0, 0, 4, L'N', SIXEL_BLUE);
        verify_sixel_row(result, 4, 0, 4, L'N', SIXEL_RED);
        verify_sixel_row(result, 8, 0, 4, L'N', SIXEL_GREEN);

        verify_sixel_row(result, 0, 0, 4, L'o', SIXEL_GREEN);
        verify_sixel_row(result, 4, 0, 4, L'o', SIXEL_BLUE);
        verify_sixel_row(result, 8, 0, 4, L'o', SIXEL_RED);
    }

    TEST_METHOD(TestParseNewLine)
    {
        SixelParser parser(L"#1NNNN#2NNNN#3NNNN-oooo#1oooo#2oooo");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 16);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 16);

        verify_sixel_row(result, 0, 0, 4, L'N', SIXEL_BLUE);
        verify_sixel_row(result, 4, 0, 4, L'N', SIXEL_RED);
        verify_sixel_row(result, 8, 0, 4, L'N', SIXEL_GREEN);

        verify_sixel_row(result, 0, 6, 4, L'o', SIXEL_GREEN);
        verify_sixel_row(result, 4, 6, 4, L'o', SIXEL_BLUE);
        verify_sixel_row(result, 8, 6, 4, L'o', SIXEL_RED);
    }

    TEST_METHOD(TestParseRepeatIntroducer)
    {
        SixelParser parser(L"#1!4N");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 4);

        verify_sixel_row(result, 0, 0, 4, L'N', SIXEL_BLUE);
    }

    TEST_METHOD(TestParseColorIntroducerRgb)
    {
        SixelParser parser(L"#0;2;0;0;0#1;2;100;100;0#2;2;0;100;0#1!4N#2!4N");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 8);

        verify_sixel_row(result, 0, 0, 4, L'N', til::color::from_xrgb(100, 100, 0));
        verify_sixel_row(result, 4, 0, 4, L'N', til::color::from_xrgb(0, 100, 0));
    }

    TEST_METHOD(TestParseColorIntroducerHsl)
    {
        SixelParser parser(L"#0;1;0;0;0#1;1;60;100;50#2;1;240;100;50#1!4N#2!4N");
        auto result = parser.GetBitmapData();
        auto height = result.size();
        VERIFY_ARE_EQUAL(height, 8);
        auto row0 = result.at(0);
        auto width = row0.size();
        VERIFY_ARE_EQUAL(width, 8);

        verify_sixel_row(result, 0, 0, 4, L'N', til::color::from_hsl(60, 100, 50));
        verify_sixel_row(result, 4, 0, 4, L'N', til::color::from_hsl(240, 100, 50));
    }

    void verify_sixel_row(
        std::vector<std::vector<til::color>>& data,
        size_t posX,
        size_t posY,
        size_t length,
        const wchar_t sixelChar,
        til::color color)
    {
        for (auto i = posX; i < length; i++)
        {
            verify_sixel(data, i, posY, sixelChar, color);
        }
    }

    void verify_sixel(
        std::vector<std::vector<til::color>>& data,
        size_t posX,
        size_t posY,
        const wchar_t sixelChar,
        til::color color)
    {
        VERIFY_IS_TRUE(data.size() >= 6);
        auto row0 = data.at(posY);
        auto row1 = data.at(posY + 1);
        auto row2 = data.at(posY + 2);
        auto row3 = data.at(posY + 3);
        auto row4 = data.at(posY + 4);
        auto row5 = data.at(posY + 5);

        if (sixel_mask(sixelChar) & 0b000001)
        {
            VERIFY_ARE_EQUAL(row0.at(posX), color);
        }

        if (sixel_mask(sixelChar) & 0b000010)
        {
            VERIFY_ARE_EQUAL(row1.at(posX), color);
        }

        if (sixel_mask(sixelChar) & 0b000100)
        {
            VERIFY_ARE_EQUAL(row2.at(posX), color);
        }

        if (sixel_mask(sixelChar) & 0b001000)
        {
            VERIFY_ARE_EQUAL(row3.at(posX), color);
        }

        if (sixel_mask(sixelChar) & 0b010000)
        {
            VERIFY_ARE_EQUAL(row4.at(posX), color);
        }

        if (sixel_mask(sixelChar) & 0b100000)
        {
            VERIFY_ARE_EQUAL(row5.at(posX), color);
        }
    }
};
