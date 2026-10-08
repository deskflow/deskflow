/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <CoreGraphics/CoreGraphics.h>

namespace deskflow::osx {

inline constexpr int64_t kMouseEventMarker = 0x4465736b666c6f77;

inline void markMouseEvent(CGEventRef event)
{
  CGEventSetIntegerValueField(event, kCGEventSourceUserData, kMouseEventMarker);
}

inline bool shouldRestoreCursor(CGEventType type, CGEventRef event)
{
  switch (type) {
  case kCGEventMouseMoved:
  case kCGEventLeftMouseDragged:
  case kCGEventRightMouseDragged:
  case kCGEventOtherMouseDragged:
    // Untagged input includes local devices and other applications' input.
    return event && CGEventGetIntegerValueField(event, kCGEventSourceUserData) != kMouseEventMarker;
  default:
    return false;
  }
}

} // namespace deskflow::osx
