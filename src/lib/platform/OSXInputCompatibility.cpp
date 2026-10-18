/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXInputCompatibility.h"

#include <libproc.h>

namespace deskflow::platform::osx {

bool isOptionsAgentPath(std::string_view path)
{
  const auto slash = path.find_last_of('/');
  return path.substr(slash == std::string_view::npos ? 0 : slash + 1) == "logioptionsplus_agent";
}

bool isOptionsAgentEvent(CGEventRef event)
{
  if (!event)
    return false;
  const auto pid = static_cast<pid_t>(CGEventGetIntegerValueField(event, kCGEventSourceUnixProcessID));
  char path[PROC_PIDPATHINFO_MAXSIZE] = {};
  return pid > 0 && proc_pidpath(pid, path, sizeof(path)) > 0 && isOptionsAgentPath(path);
}

bool shouldRelayMiddleClick(CGEventType type, CGEventRef event, bool isOnComputer, bool enabled, bool optionsSource)
{
  return enabled && !isOnComputer && optionsSource && event &&
         (type == kCGEventOtherMouseDown || type == kCGEventOtherMouseUp) &&
         CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) == kCGMouseButtonCenter &&
         CGEventGetIntegerValueField(event, kCGEventSourceUserData) != kCompatibilityEventTag;
}

bool shouldSyncShiftRelease(CGEventType type, CGEventRef event, bool isOnComputer, bool enabled)
{
  if (!enabled || isOnComputer || !event || type != kCGEventFlagsChanged ||
      CGEventGetIntegerValueField(event, kCGEventSourceUserData) == kCompatibilityEventTag ||
      CGEventGetIntegerValueField(event, kCGEventSourceUnixProcessID) != 0 ||
      (CGEventGetFlags(event) & kCGEventFlagMaskShift) != 0)
    return false;
  const auto key = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  return key == 56 || key == 60;
}

CGEventRef copyCompatibilityEvent(CGEventRef event)
{
  auto copy = event ? CGEventCreateCopy(event) : nullptr;
  if (copy)
    CGEventSetIntegerValueField(copy, kCGEventSourceUserData, kCompatibilityEventTag);
  return copy;
}

} // namespace deskflow::platform::osx
