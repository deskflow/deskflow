/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "UnicodeTests.h"

#include "base/Unicode.h"

void UnicodeTests::initTestCase()
{
  m_log.setFilter(LogLevel::Debug2);
}

void UnicodeTests::UTF16ToUTF8()
{
  bool errors;
  auto result = Unicode::UTF16ToUTF8(std::string("h\0e\0l\0l\0o\0", 10), &errors);

  QVERIFY(!errors);
  QCOMPARE(result.c_str(), "hello");
}

QTEST_MAIN(UnicodeTests)
