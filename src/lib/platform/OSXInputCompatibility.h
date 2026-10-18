/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <cstdint>
#include <string_view>

namespace deskflow::platform::osx {

inline constexpr int64_t kCompatibilityEventTag = 0x44464d58;

bool isOptionsAgentPath(std::string_view path);
bool isOptionsAgentEvent(CGEventRef event);
bool shouldRelayMiddleClick(CGEventType type, CGEventRef event, bool isOnComputer, bool enabled, bool optionsSource);
bool shouldSyncShiftRelease(CGEventType type, CGEventRef event, bool isOnComputer, bool enabled);
CGEventRef copyCompatibilityEvent(CGEventRef event);

} // namespace deskflow::platform::osx
