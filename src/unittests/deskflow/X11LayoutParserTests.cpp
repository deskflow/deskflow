/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "X11LayoutParserTests.h"

#include "deskflow/unix/X11LayoutsParser.h"

void X11LayoutParserTests::convertLayouts()
{
  // An empty layout name yields an empty result.
  QCOMPARE(X11LayoutsParser::convertLayoutToISO(""), "");

  // An unknown layout name is not present in the registry.
  QCOMPARE(X11LayoutsParser::convertLayoutToISO("notARealLayout"), "");

  // A well-known layout resolves to its ISO 639-1 code. "us" is always
  // present in the xkeyboard-config data and maps to "eng" -> "en".
  QCOMPARE(X11LayoutsParser::convertLayoutToISO("us"), "en");
}

QTEST_MAIN(X11LayoutParserTests)
