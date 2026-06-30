/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/HidConsumer.h"

#include "client/MouserClient.h"
#include "common/Settings.h"

namespace deskflow::client {

std::string encodeHidReportAsMouserLine(uint16_t deviceId, std::string_view bytes)
{
  static const char *digits = "0123456789abcdef";
  std::string hex;
  hex.reserve(bytes.size() * 2);
  for (const char c : bytes) {
    const auto byte = static_cast<unsigned char>(c);
    hex.push_back(digits[byte >> 4]);
    hex.push_back(digits[byte & 0xF]);
  }
  return R"({"type": "report", "device_id": )" + std::to_string(deviceId) + R"(, "data": ")" + hex + R"("})";
}

HidConsumerMode hidConsumerModeFromSettings()
{
  const auto mode = Settings::value(Settings::Client::HidConsumer).toString();
  if (mode == QStringLiteral("none")) {
    return HidConsumerMode::None;
  }
  if (Settings::value(Settings::Client::MouserEnabled).toBool()) {
    return HidConsumerMode::Mouser;
  }
  return HidConsumerMode::None;
}

MouserHidConsumer::MouserHidConsumer(MouserClient *client) : m_client(client)
{
}

void MouserHidConsumer::deliverRawReport(uint16_t deviceId, const std::string &bytes)
{
  if (m_client == nullptr || bytes.empty()) {
    return;
  }
  m_client->deliver(encodeHidReportAsMouserLine(deviceId, bytes));
}

} // namespace deskflow::client
