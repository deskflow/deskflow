/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXTrackpadGesture.h"

#include <cmath>
#include <utility>

namespace deskflow::osx::gesture {
namespace {

constexpr auto kFieldHIDType = static_cast<CGEventField>(110);
constexpr auto kFieldMotion = static_cast<CGEventField>(123);
constexpr auto kFieldTargetPSN = static_cast<CGEventField>(39);
constexpr auto kFieldTargetPID = static_cast<CGEventField>(40);
constexpr int64_t kHIDTypeRotation = 5;
constexpr int64_t kHIDTypePinch = 8;
constexpr int64_t kMotionTap = 5;

bool isTrackpadScroll(CGEventRef event)
{
  return CGEventGetIntegerValueField(event, kCGScrollWheelEventIsContinuous) != 0 &&
         (CGEventGetIntegerValueField(event, kCGScrollWheelEventScrollPhase) != 0 ||
          CGEventGetIntegerValueField(event, kCGScrollWheelEventMomentumPhase) != 0);
}

std::string serialize(CGEventRef event)
{
  CFDataRef data = CGEventCreateData(kCFAllocatorDefault, event);
  if (data == nullptr) {
    return {};
  }
  std::string bytes(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
  CFRelease(data);
  return bytes;
}

bool supported(CGEventType type, CGEventRef event)
{
  if (type == kCGEventScrollWheel) {
    return isTrackpadScroll(event);
  }
  if (static_cast<int>(type) == kEventTypeDockControl) {
    return CGEventGetIntegerValueField(event, kFieldMotion) != kMotionTap;
  }
  if (static_cast<int>(type) == kEventTypeGesture) {
    const auto hidType = CGEventGetIntegerValueField(event, kFieldHIDType);
    return hidType == kHIDTypePinch || hidType == kHIDTypeRotation;
  }
  return false;
}

} // namespace

std::vector<std::string> Forwarder::take(CGEventType type, CGEventRef event)
{
  if (!supported(type, event)) {
    return {};
  }
  auto bytes = serialize(event);
  if (bytes.empty()) {
    return {};
  }
  return {std::move(bytes)};
}

std::optional<std::string> takeScroll(CGEventRef event)
{
  if (!isTrackpadScroll(event)) {
    return std::nullopt;
  }
  auto bytes = serialize(event);
  if (bytes.empty()) {
    return std::nullopt;
  }
  return bytes;
}

void scaleScroll(CGEventRef event, double x, double y)
{
  if (x == 1 && y == 1) {
    return;
  }
  // setting a coarser delta recomputes the finer ones, so read them all
  // first and write the line, fixed-point and point deltas in that order
  const auto lineY = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);
  const auto lineX = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2);
  const auto fixedY = CGEventGetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1);
  const auto fixedX = CGEventGetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis2);
  const auto pointY = CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1);
  const auto pointX = CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2);
  const auto scaled = [](int64_t value, double scale) { return std::llround(static_cast<double>(value) * scale); };
  CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1, scaled(lineY, y));
  CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2, scaled(lineX, x));
  CGEventSetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1, fixedY * y);
  CGEventSetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis2, fixedX * x);
  CGEventSetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1, scaled(pointY, y));
  CGEventSetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2, scaled(pointX, x));
}

bool post(const std::string &bytes, const PostOptions &options)
{
  CFDataRef data = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(bytes.data()),
                              static_cast<CFIndex>(bytes.size()));
  if (data == nullptr) {
    return false;
  }
  CGEventRef event = CGEventCreateFromData(kCFAllocatorDefault, data);
  CFRelease(data);
  if (event == nullptr) {
    return false;
  }
  // Also reject excluded input from a peer still running an older build.
  if (!supported(CGEventGetType(event), event)) {
    CFRelease(event);
    return true;
  }

  // Restore the established session-stage replay. Leave the embedded Dock
  // gesture record unchanged; only the public routing fields are local.
  if (CGEventRef now = CGEventCreate(nullptr); now != nullptr) {
    CGEventSetTimestamp(event, CGEventGetTimestamp(now));
    CGEventSetLocation(event, CGEventGetLocation(now));
    CFRelease(now);
  }
  CGEventSetIntegerValueField(event, kFieldTargetPSN, 0);
  CGEventSetIntegerValueField(event, kFieldTargetPID, 0);
  if (CGEventGetType(event) == kCGEventScrollWheel) {
    scaleScroll(event, options.scrollScaleX, options.scrollScaleY);
    CGEventSetFlags(event, options.flags);
    CGEventPost(kCGHIDEventTap, event);
  } else {
    CGEventPost(kCGSessionEventTap, event);
  }
  CFRelease(event);
  return true;
}

} // namespace deskflow::osx::gesture
