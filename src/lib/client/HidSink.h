/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace deskflow::client {

//! Magic prefix for binary HID report frames on the Mouser loopback sink.
inline constexpr char kHidSinkMagic[4] = {'D', 'F', 'H', 'R'};
inline constexpr size_t kHidSinkHeaderBytes = 10; // magic + deviceId + payloadLen

//! Encode one raw HID report as a binary DFHR frame (device-agnostic sink wire format).
[[nodiscard]] std::string encodeHidReportFrame(uint16_t deviceId, std::string_view payload);

//! True when ``data`` begins with the DFHR magic prefix.
[[nodiscard]] bool isHidReportFrame(std::string_view data);

} // namespace deskflow::client
