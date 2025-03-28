/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureUtilsTests.h"

#include "net/SecureUtils.h"

using namespace deskflow;

void SecureUtilsTests::checkHex()
{
  QByteArray fingerprint(
      "\x28\xFD\x0A\x98\x8A\x0E\xA1\x6C\xD7\xE8\x6C\xA7\xEE\x58\x41\x71\xCA\xB2\x8E\x49\x25\x94\x90\x25\x26\x05\x8D\xAF"
      "\x63\xED\x2E\x30",
      32
  );
  QCOMPARE(
      deskflow::formatSSLFingerprint(fingerprint, true),
      "28:FD:0A:98:8A:0E:A1:6C:D7:E8:6C:A7:EE:58:41:71:CA:B2:8E:49:25:94:90:25:26:05:8D:AF:63:ED:2E:30"
  );
}

void SecureUtilsTests::checkArt()
{
  QByteArray fingerprint(
      "\x28\xFD\x0A\x98\x8A\x0E\xA1\x6C\xD7\xE8\x6C\xA7\xEE\x58\x41\x71\xCA\xB2\x8E\x49\x25\x94\x90\x25\x26\x05\x8D\xAF"
      "\x63\xED\x2E\x30",
      32
  );
  QCOMPARE(
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

QTEST_MAIN(SecureUtilsTests)
