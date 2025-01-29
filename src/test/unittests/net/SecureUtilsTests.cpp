/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Devlopers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/SecureUtils.h"

#include <gtest/gtest.h>
#include <random>

namespace {

std::vector<uint8_t> generateBytes(std::size_t seed, std::size_t size)
{
  std::mt19937_64 engine{seed};
  std::uniform_int_distribution<int> dist{0, 255};
  std::vector<uint8_t> bytes;

  bytes.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    bytes.push_back(dist(engine));
  }

  return bytes;
}

} // namespace

TEST(SecureUtilsTest, FormatSslFingerprintHexWithSeparators)
{
  auto fingerprint = generateBytes(0, 32);
  ASSERT_EQ(
      deskflow::formatSSLFingerprint(fingerprint, true),
      "28:FD:0A:98:8A:0E:A1:6C:D7:E8:6C:A7:EE:58:41:71:CA:B2:8E:49:25:94:90:25:26:05:8D:AF:63:ED:2E:30"
  );
}
