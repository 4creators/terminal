// SPDX-License-Identifier: AGPL-3.0-only
// Copyright © 2026 Jacek Błaszczyński

#include "precomp.h"
#include "DriverHook.hpp"

#include "../types/inc/convert.hpp"

#include <TraceLoggingProvider.h>
TRACELOGGING_DECLARE_PROVIDER(g_hConhostV2EventTraceProvider);

// Routine Description:
// - Initializes the connection to the programmatic ConDriver telemetry pipe.
// Arguments:
// - pipeName - The name of the named pipe (e.g., L"condriver-telemetry-1234")
void DriverHook::Initialize(const std::wstring& pipeName)
{
    // If the orchestrator didn't provide a pipe, or we're already connected, do nothing.
    if (pipeName.empty() || _hPipe)
    {
        return;
    }

    TraceLoggingWrite(
        g_hConhostV2EventTraceProvider,
        "DriverHook_Initialize",
        TraceLoggingWideString(pipeName.c_str(), "PipeName"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TIL_KEYWORD_TRACE));

    // Connect to the named pipe created by the Rust orchestrator
    // We open it with GENERIC_WRITE access since we only stream telemetry out
    _hPipe.reset(CreateFileW(
        pipeName.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));

    // We don't throw or crash if we fail to connect; we just proceed silently
    // without telemetry, allowing the terminal to function normally.
}

// Routine Description:
// - Writes the unencoded plain text to the telemetry pipe.
// Arguments:
// - text - A chunk of plain text extracted from the TextBuffer.
// - isVt - Whether the text is being written via the modern VT engine (true) or Legacy API (false).
void DriverHook::WriteStream(const std::wstring_view text, const bool isVt)
{
    if (!_hPipe)
    {
        return;
    }

    // The named pipe is opened in BYTE mode, and the orchestrator expects UTF-8
    std::string utf8Text;
    try
    {
        utf8Text = ConvertToA(CP_UTF8, text);
    }
    catch (...)
    {
        return;
    }
    
    if (utf8Text.empty())
    {
        return;
    }

    if (_transmissions == 0)
    {
        TraceLoggingWrite(
            g_hConhostV2EventTraceProvider,
            "DriverHook_FirstTransmission",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(TIL_KEYWORD_TRACE));
    }

    DWORD written = 0;
    const auto result = WriteFile(
        _hPipe.get(),
        utf8Text.data(),
        gsl::narrow_cast<DWORD>(utf8Text.size()),
        &written,
        nullptr);

    if (result)
    {
        _transmissions++;
        _bytesTransmitted += written;

        if (isVt)
        {
            TraceLoggingWrite(
                g_hConhostV2EventTraceProvider,
                "DriverHook_WriteStream_VT",
                TraceLoggingUInt64(_transmissions, "Transmissions"),
                TraceLoggingUInt64(_bytesTransmitted, "BytesTransmitted"),
                TraceLoggingUInt32(written, "BytesWritten"),
                TraceLoggingString(utf8Text.c_str(), "Payload"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                TraceLoggingKeyword(TIL_KEYWORD_TRACE));
        }
        else
        {
            TraceLoggingWrite(
                g_hConhostV2EventTraceProvider,
                "DriverHook_WriteStream_Legacy",
                TraceLoggingUInt64(_transmissions, "Transmissions"),
                TraceLoggingUInt64(_bytesTransmitted, "BytesTransmitted"),
                TraceLoggingUInt32(written, "BytesWritten"),
                TraceLoggingString(utf8Text.c_str(), "Payload"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                TraceLoggingKeyword(TIL_KEYWORD_TRACE));
        }
    }
    else
    {
        const auto lastError = GetLastError();
        // ERROR_BROKEN_PIPE means the ConDriver orchestrator closed its end or exited.
        // We gracefully disconnect our handle to prevent further write attempts.
        if (lastError == ERROR_BROKEN_PIPE)
        {
            _hPipe.reset();
        }
    }
}
