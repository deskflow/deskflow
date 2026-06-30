/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/HidSink.h"

#include <cstring>

namespace deskflow::client {

namespace {

void appendLe16(std::string &out, uint16_t value)
{
  out.push_back(static_cast<char>(value & 0xFF));
  out.push_back(static_cast<char>((value >> 8) & 0xFF));
}

void appendLe32(std::string &out, uint32_t value)
{
  out.push_back(static_cast<char>(value & 0xFF));
  out.push_back(static_cast<char>((value >> 8) & 0xFF));
  out.push_back(static_cast<char>((value >> 16) & 0xFF));
  out.push_back(static_cast<char>((value >> 24) & 0xFF));
}

} // namespace

std::string encodeHidReportFrame(uint16_t deviceId, std::string_view payload)
{
  std::string frame;
  frame.reserve(kHidSinkHeaderBytes + payload.size());
  frame.append(kHidSinkMagic, sizeof(kHidSinkMagic));
  appendLe16(frame, deviceId);
  appendLe32(frame, static_cast<uint32_t>(payload.size()));
  frame.append(payload.data(), payload.size());
  return frame;
}

bool isHidReportFrame(std::string_view data)
{
  return data.size() >= sizeof(kHidSinkMagic)
         && std::memcmp(data.data(), kHidSinkMagic, sizeof(kHidSinkMagic)) == 0;
}

} // namespace deskflow::client
