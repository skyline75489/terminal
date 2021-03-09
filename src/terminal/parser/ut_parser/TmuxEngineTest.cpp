// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "WexTestClass.h"
#include "../../inc/consoletaeftemplates.hpp"

#include "TmuxEngine.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class TmuxEngineTest;
        };
    };
};

using namespace Microsoft::Console::VirtualTerminal;

class Microsoft::Console::VirtualTerminal::TmuxEngineTest
{
    TEST_CLASS(TmuxEngineTest);

    TEST_CLASS_SETUP(ClassSetup)
    {
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        return true;
    }

    TEST_METHOD(TestOutputBegin);
};

void TmuxEngineTest::TestOutputBegin()
{
    auto enginePtr = std::make_unique<TmuxEngine>();
    auto& engine{ *enginePtr.get() };

    engine.ProcessString(L"%begin 1615267328 6271 0");
}
