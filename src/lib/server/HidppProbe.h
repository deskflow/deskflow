/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QJsonObject>

#include <cstdint>
#include <map>
#include <string>

namespace deskflow::server {

//! Runtime-discovered HID++ decode context for passthrough connect lines.
struct HidppDecodeContext
{
  int featIdx = -1;
  uint16_t gestureCid = 0;
  bool rawxy = true;
  std::map<uint16_t, std::string> extraDiverts;
  bool valid() const
  {
    return featIdx > 0 && featIdx <= 0xFF;
  }
};

//! True when this platform implements a live HID++ probe.
[[nodiscard]] bool hidppProbeCapable();

//! Probe the physical device for REPROG_V4 feature index and catalog metadata.
[[nodiscard]] HidppDecodeContext probeHidppDecode(uint16_t vendorId, uint16_t productId);

//! Convert a probe result into the Mouser-compatible decode JSON object.
[[nodiscard]] QJsonObject hidppDecodeToJson(const HidppDecodeContext &context);

} // namespace deskflow::server
