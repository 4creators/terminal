// SPDX-License-Identifier: AGPL-3.0-only
// Copyright © 2026 Jacek Błaszczyński

#pragma once

#include <string>
#include <string_view>
#include <wil/resource.h>

class DriverHook
{
public:
    DriverHook() = default;
    ~DriverHook() = default;

    // Non-copyable, non-movable
    DriverHook(const DriverHook&) = delete;
    DriverHook& operator=(const DriverHook&) = delete;
    DriverHook(DriverHook&&) = delete;
    DriverHook& operator=(DriverHook&&) = delete;

    void Initialize(const std::wstring& pipeName);
    void WriteStream(const std::wstring_view text, const bool isVt);

    uint64_t GetTransmissionsCount() const noexcept { return _transmissions; }
    uint64_t GetBytesTransmitted() const noexcept { return _bytesTransmitted; }

private:
    wil::unique_hfile _hPipe;
    uint64_t _transmissions{ 0 };
    uint64_t _bytesTransmitted{ 0 };
};
