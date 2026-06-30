/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

class MouserClient;

namespace deskflow::client {

inline constexpr size_t kMaxHidReportPayloadBytes = 4096;

//! True when raw HID reports should be delivered to the local Mouser loopback client.
[[nodiscard]] bool mouserHidDeliveryEnabled();

//! Encode a raw HID report frame as a Mouser loopback JSON line.
std::string encodeHidReportAsMouserLine(uint16_t deviceId, std::string_view bytes);

[[nodiscard]] bool shouldDeliverRawHidReport(size_t byteCount);

//! Deliver an encoded report line when payload size is valid; no-op when deliver is empty.
template<typename DeliverFn>
void deliverRawHidReport(DeliverFn &&deliver, uint16_t deviceId, const std::string &bytes)
{
  if (!shouldDeliverRawHidReport(bytes.size())) {
    return;
  }
  deliver(encodeHidReportAsMouserLine(deviceId, bytes));
}

void deliverRawHidReportToMouser(MouserClient *client, uint16_t deviceId, const std::string &bytes);

} // namespace deskflow::client
