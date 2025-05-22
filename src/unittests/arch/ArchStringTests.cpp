/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ArchStringTests.h"

#include "arch/ArchString.h"

void ArchStringTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void ArchStringTests::convertStringWCToMB_buffer()
{
  ArchString as;
  char buff[20];
  bool errors;

  auto converted = as.convStringWCToMB(buff, L"Hello", 6, &errors);

  QCOMPARE(converted, 6);
  QCOMPARE(buff, "Hello");
  QVERIFY(!errors);
}

void ArchStringTests::convertStringWCToMB_noBuffer()
{
  ArchString as;
  bool errors;

  auto converted = as.convStringWCToMB(nullptr, L"Hello", 6, &errors);

  QCOMPARE(converted, 6);
  QVERIFY(!errors);
}

void ArchStringTests::convertStringMBToWC()
{
  ArchString as;
  wchar_t buff[20];
  bool errors;

  auto converted = as.convStringMBToWC(buff, "Hello", 6, &errors);
  QCOMPARE(converted, 6);

  auto actual = QString::fromStdWString(buff);
  auto expected = QString::fromStdWString(L"Hello");

  QCOMPARE(actual, expected);
  QVERIFY(!errors);
}

QTEST_MAIN(ArchStringTests)
