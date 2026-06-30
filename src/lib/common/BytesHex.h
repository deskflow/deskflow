/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace deskflow {

//! Lowercase hex encoding for raw byte buffers (no separators).
inline std::string bytesToLowerHex(std::string_view bytes)
{
  static const char *digits = "0123456789abcdef";
  std::string hex;
  hex.reserve(bytes.size() * 2);
  for (const char c : bytes) {
    const auto byte = static_cast<unsigned char>(c);
    hex.push_back(digits[byte >> 4]);
    hex.push_back(digits[byte & 0xF]);
  }
  return hex;
}

inline std::string bytesToLowerHex(const uint8_t *bytes, size_t len)
{
  return bytesToLowerHex(std::string_view(reinterpret_cast<const char *>(bytes), len));
}

} // namespace deskflow
