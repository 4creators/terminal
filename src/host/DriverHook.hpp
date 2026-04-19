// SPDX-License-Identifier: AGPL-3.0-only
// Copyright © 2026 Jacek Błaszczyński

#pragma once

#include "precomp.h"
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
    void WriteStream(const std::wstring_view text);

private:
    wil::unique_hfile _hPipe;
};
