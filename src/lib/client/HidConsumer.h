/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class MouserClient;

namespace deskflow::client {

//! Encode a raw HID report frame as a Mouser loopback JSON line.
std::string encodeHidReportAsMouserLine(uint16_t deviceId, std::string_view bytes);

enum class HidConsumerMode
{
  None,
  Mouser,
};

HidConsumerMode hidConsumerModeFromSettings();

//! Delivers raw HID report bytes to a local consumer (Mouser loopback today).
class HidConsumer
{
public:
  virtual ~HidConsumer() = default;
  virtual void deliverRawReport(uint16_t deviceId, const std::string &bytes) = 0;
};

class MouserHidConsumer : public HidConsumer
{
public:
  explicit MouserHidConsumer(MouserClient *client);

  void deliverRawReport(uint16_t deviceId, const std::string &bytes) override;

private:
  MouserClient *m_client;
};

} // namespace deskflow::client
