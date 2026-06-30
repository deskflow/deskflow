/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/HidConsumer.h"

#include "client/HidSink.h"
#include "client/MouserClient.h"
#include "common/BytesHex.h"
#include "common/Settings.h"

namespace deskflow::client {

bool mouserHidDeliveryEnabled()
{
  return Settings::value(Settings::Client::MouserEnabled).toBool();
}

std::string encodeHidReportAsMouserLine(uint16_t deviceId, std::string_view bytes)
{
  return R"({"type": "report", "device_id": )" + std::to_string(deviceId) + R"(, "data": ")"
         + deskflow::bytesToLowerHex(bytes) + R"("})";
}

std::string encodeHidReportAsSinkFrame(uint16_t deviceId, std::string_view bytes)
{
  return encodeHidReportFrame(deviceId, bytes);
}

bool shouldDeliverRawHidReport(size_t byteCount)
{
  return byteCount > 0 && byteCount <= kMaxHidReportPayloadBytes;
}

void deliverRawHidReportToMouser(MouserClient *client, uint16_t deviceId, const std::string &bytes)
{
  if (client == nullptr) {
    return;
  }
  deliverRawHidReport([client](const std::string &frame) { client->deliverReport(frame); }, deviceId, bytes);
}

} // namespace deskflow::client
