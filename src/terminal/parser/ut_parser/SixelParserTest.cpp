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

class Microsoft::Console::VirtualTerminal::SixelParserTest
{
    TEST_CLASS(SixelParserTest);

    TEST_METHOD(TestParse)
    {
        SixelParser parser(L"#1NNNN#2NNNN#3NNNN$oooo#1oooo#2oooo-#3BBBB#1BBBB#2BBBB${{{{#3{{{{#1{{{{????");
    }
};

