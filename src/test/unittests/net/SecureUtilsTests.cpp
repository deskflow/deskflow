/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/SecureUtils.h"

#include <gtest/gtest.h>

TEST(SecureUtilsTest, formatSSLFingerprints_fromHex_withSeperators)
{
  std::vector<uint8_t> fingerprint = {40,  253, 10,  152, 138, 14,  161, 108, 215, 232, 108, 167, 238, 88,  65, 113,
                                      202, 178, 142, 73,  37,  148, 144, 37,  38,  5,   141, 175, 99,  237, 46, 48};

  ASSERT_EQ(
      deskflow::formatSSLFingerprint(fingerprint, true),
      "28:FD:0A:98:8A:0E:A1:6C:D7:E8:6C:A7:EE:58:41:71:CA:B2:8E:49:25:94:90:25:26:05:8D:AF:63:ED:2E:30"
  );
}

TEST(SecureUtilsTest, createFingerprintArt)
{
  std::vector<uint8_t> fingerprint = {40,  253, 10,  152, 138, 14,  161, 108, 215, 232, 108, 167, 238, 88,  65, 113,
                                      202, 178, 142, 73,  37,  148, 144, 37,  38,  5,   141, 175, 99,  237, 46, 48};
  ASSERT_EQ(
      deskflow::generateFingerprintArt(fingerprint), "╔═════════════════╗\n"
                                                     "║*X+. .           ║\n"
                                                     "║*oo +            ║\n"
                                                     "║ + =             ║\n"
                                                     "║  B  . .         ║\n"
                                                     "║.+... o S        ║\n"
                                                     "║E+ ++. .         ║\n"
                                                     "║B*++..  .        ║\n"
                                                     "║+o*o o .         ║\n"
                                                     "║+o*Bo .          ║\n"
                                                     "╚═════════════════╝"
  );
}
