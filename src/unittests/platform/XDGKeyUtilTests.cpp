/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "XDGKeyUtilTests.h"

#include "platform/XDGKeyUtil.h"

void XDGKeyUtilTests::mapKeySymToKeyID_unicodeKeySym_mapsToCodePoint_data()
{
  // xkb writes characters without a legacy keysym as 0x01000000 | code point
  QTest::addColumn<quint32>("keySym");
  QTest::addColumn<quint32>("keyID");
  QTest::newRow("s with comma below") << 0x01000219u << 0x0219u;
  QTest::newRow("t with comma below") << 0x0100021bu << 0x021bu;
  QTest::newRow("small tilde") << 0x010002dcu << 0x02dcu;
  QTest::newRow("legacy abreve still mapped") << 0x01e3u << 0x0103u;
}

void XDGKeyUtilTests::mapKeySymToKeyID_unicodeKeySym_mapsToCodePoint()
{
  QFETCH(quint32, keySym);
  QFETCH(quint32, keyID);

  QCOMPARE(XDGKeyUtil::mapKeySymToKeyID(keySym), keyID);
}

QTEST_GUILESS_MAIN(XDGKeyUtilTests)
