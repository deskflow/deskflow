/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "UnicodeTests.h"

#include "../../lib/base/Unicode.h"

#include "arch/IArchString.h"

void UnicodeTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void UnicodeTests::UTF32ToUTF8()
{
  bool errors;
  auto result = Unicode::UTF32ToUTF8(std::string("h\0\0\0e\0\0\0l\0\0\0l\0\0\0o\0\0\0", 20), &errors);

  QVERIFY(!errors);
  QCOMPARE(result.c_str(), "hello");
}

void UnicodeTests::UTF16ToUTF8()
{
  bool errors;
  auto result = Unicode::UTF16ToUTF8(std::string("h\0e\0l\0l\0o\0", 10), &errors);

  QVERIFY(!errors);
  QCOMPARE(result.c_str(), "hello");
}

void UnicodeTests::UCS2ToUTF8_kUCS2()
{
  bool errors;
  auto result = Unicode::textToUTF8("hello", &errors, IArchString::kUCS2);

  QVERIFY(!errors);
#ifdef _WIN32
  QCOMPARE(result, std::string("hello", 5)); // mixed-platform expected result
#else
  QCOMPARE(result, std::string("h\0e\0l", 5)); // mixed-platform expected result
#endif // _WIN32
}

void UnicodeTests::UCS2ToUTF8()
{
  bool errors;
  auto result = Unicode::textToUTF8("hello", &errors);

  QVERIFY(!errors);
  QCOMPARE(result, std::string("hello", 5)); // mixed-platform expected result
}

QTEST_MAIN(UnicodeTests)
