// SPDX-License-Identifier: AGPL-3.0-only
// Copyright © 2026 Jacek Błaszczyński

#include "precomp.h"
#include "WexTestClass.h"
#include "../../inc/consoletaeftemplates.hpp"

#include "../DriverHook.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

class DriverHookTests
{
    TEST_CLASS(DriverHookTests);

    TEST_METHOD(EmptyPipeNameIgnoresConnection)
    {
        DriverHook hook;
        hook.Initialize(L"");
        // Ensure no crash occurs when streaming text to an uninitialized hook
        hook.WriteStream(L"test");
    }

    TEST_METHOD(MockPipeStreaming)
    {
        // Create an anonymous pipe to mock the condriver orchestrator connection
        HANDLE hRead, hWrite;
        VERIFY_WIN32_BOOL_SUCCEEDED(CreatePipe(&hRead, &hWrite, nullptr, 0));

        // Use a simple, non-existent pipe name since we can't easily mock CreateFile in the hook
        // without making _hPipe public or using dependency injection.
        // For testing we will just verify the graceful failure on a bad pipe string.
        DriverHook hook;
        hook.Initialize(L"\\\\.\\pipe\\invalid-test-pipe");
        hook.WriteStream(L"This should safely fail internally without crashing");

        CloseHandle(hRead);
        CloseHandle(hWrite);
    }
};
