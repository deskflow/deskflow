/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <optional>
#include <string>
#include <vector>

namespace deskflow::osx::gesture {

inline constexpr int kEventTypeGesture = 29;
inline constexpr int kEventTypeDockControl = 30;

inline bool isGestureEventType(CGEventType type)
{
  return static_cast<int>(type) == kEventTypeGesture || static_cast<int>(type) == kEventTypeDockControl;
}

//! Selects Dock swipes, application pinch and rotation events.
//! Raw touch frames and three-finger tap markers are excluded.
class Forwarder
{
public:
  std::vector<std::string> take(CGEventType type, CGEventRef event);
};

//! Serializes a trackpad scroll: continuous, in a scroll or momentum phase
//! (trackpads and Magic Mouse). AppKit and browsers need the phases for
//! smooth scrolling, rubber-banding and two-finger swipes between pages.
//! Other scroll events have no such data and travel as wheel deltas.
std::optional<std::string> takeScroll(CGEventRef event);

//! How the secondary adjusts posted scrolls
struct PostOptions
{
  double scrollScaleX = 1; //!< negative inverts, as the client's scroll settings
  double scrollScaleY = 1;
  CGEventFlags flags = 0; //!< the secondary's modifier keys
};

//! Multiplies the deltas of a scroll event
void scaleScroll(CGEventRef event, double x, double y);

//! Replays a supported serialized event. Gestures go to the session event tap
//! with the embedded gesture payload preserved for the receiving Dock; scrolls
//! go to the HID event tap like the client's other scrolls.
bool post(const std::string &bytes, const PostOptions &options = {});

} // namespace deskflow::osx::gesture
