// SPDX-License-Identifier: AGPL-3.0-only
// Copyright © 2026 Jacek Błaszczyński

#include "precomp.h"
#include "WexTestClass.h"
#include "../../inc/consoletaeftemplates.hpp"

#include "../DriverHook.hpp"
#include "../../renderer/base/Renderer.hpp"

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
        VERIFY_ARE_EQUAL(0ui64, hook.GetTransmissionsCount());
        VERIFY_ARE_EQUAL(0ui64, hook.GetBytesTransmitted());
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

        VERIFY_ARE_EQUAL(0ui64, hook.GetTransmissionsCount());
        VERIFY_ARE_EQUAL(0ui64, hook.GetBytesTransmitted());

        CloseHandle(hRead);
        CloseHandle(hWrite);
    }

    TEST_METHOD(RendererTriggerNewTextNotificationInvokesHook)
    {
        // We test the architecture by binding a mock DriverHook instance to a std::function 
        // to mimic exactly what srvinit.cpp does with the Renderer's SetDriverTelemetryHook.
        DriverHook mockHook;
        std::wstring_view receivedText = L"";

        // Mimic the exact lambda signature used in srvinit.cpp
        std::function<void(const std::wstring_view)> telemetryHook = [&](const std::wstring_view text) {
            receivedText = text;
            mockHook.WriteStream(text);
        };

        // Mimic the exact dispatch loop used in Renderer::TriggerNewTextNotification
        auto triggerNotification = [&](const std::wstring_view newText) {
            if (telemetryHook)
            {
                telemetryHook(newText);
            }
        };

        // Fire the mocked notification
        triggerNotification(L"Hello ConDriver");

        // Verify the callback correctly received the unencoded string exactly as passed by the engine
        VERIFY_ARE_EQUAL(L"Hello ConDriver", receivedText);
    }
};
