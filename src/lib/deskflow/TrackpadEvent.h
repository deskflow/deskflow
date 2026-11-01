/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace deskflow {

//! A trackpad gesture event made on the primary, serialized by its platform
/*!
On macOS this is the CGEventCreateData() form of the Quartz event, which keeps
the IOHID data (finger positions, the gesture record the Dock reads) that the
public event fields lack. The active secondary posts it unchanged, so its own
Dock recognizes the gesture like a local one.

Trackpad scrolls travel the same way, with their phases, so the client
scrolls like a local trackpad and can swipe between pages.

Event data of EventTypes::PrimaryComputerTrackpadEvent; sent with
kMsgDTrackpadEvent (protocol 1.9).
*/
class TrackpadEventData : public EventData
{
public:
  //! Wheel deltas, as in PrimaryComputerWheel
  struct Wheel
  {
    int32_t x = 0;
    int32_t y = 0;
  };

  explicit TrackpadEventData(std::string bytes, std::optional<Wheel> wheel = std::nullopt)
      : m_bytes(std::move(bytes)),
        m_wheel(wheel)
  {
  }

  std::string m_bytes;

  //! For a scroll: what to send a client that cannot post trackpad events
  std::optional<Wheel> m_wheel;
};

} // namespace deskflow
