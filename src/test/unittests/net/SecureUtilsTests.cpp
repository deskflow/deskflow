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
  std::string fingerprint = "(\xFD\n\x98\x8A\x0E\xA1l\xD7\xE8l\xA7\xEEXAq\xCA\xB2\x8EI%\x94\x90%&\x05\x8D\xAF"
                            "c\xED.0";

  ASSERT_EQ(
      deskflow::formatSSLFingerprint(fingerprint, true, true), "28:FD:0A:98:8A:0E:A1:6C:D7:E8:6C:A7:EE:58:41:71:"
                                                               "CA:B2:8E:49:25:94:90:25:26:05:8D:AF:63:ED:2E:30"
  );
}
