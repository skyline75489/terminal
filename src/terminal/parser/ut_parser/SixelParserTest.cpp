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

class SixelParserTest final
{
    TEST_CLASS(SixelParserTest);

    TEST_METHOD(TestParse)
    {
        SixelParser parser(L"#1NNNN#2NNNN#3NNNN$oooo#1oooo#2oooo-#3BBBB#1BBBB#2BBBB${{{{#3{{{{#1{{{{????");
        auto result = parser.GetBitmapData();

        auto row0 = result.at(0);
        VERIFY_ARE_EQUAL(row0.at(0), SIXEL_BLUE);
        VERIFY_ARE_EQUAL(row0.at(1), SIXEL_BLUE);
        VERIFY_ARE_EQUAL(row0.at(2), SIXEL_BLUE);
    }
};

