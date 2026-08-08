/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "XkbLayoutParserTests.h"

#include "deskflow/unix/XkbLayoutsParser.h"

void XkbLayoutParserTests::convertLayouts()
{
  // An empty layout name yields an empty result.
  QCOMPARE(XkbLayoutsParser::convertLayoutToISO(""), "");

  // An unknown layout name is not present in the registry.
  QCOMPARE(XkbLayoutsParser::convertLayoutToISO("notARealLayout"), "");

  // A well-known layout resolves to its ISO 639-1 code. "us" is always
  // present in the xkeyboard-config data and maps to "eng" -> "en".
  QCOMPARE(XkbLayoutsParser::convertLayoutToISO("us"), "en");
}

QTEST_MAIN(XkbLayoutParserTests)
