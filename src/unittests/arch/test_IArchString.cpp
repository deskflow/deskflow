/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_IArchString.h"

void IArchString_Test::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void IArchString_Test::convertStringWCToMB_buffer()
{
  SampleIArchString as;
  char buff[20];
  bool errors;
  auto converted = as.convStringWCToMB(buff, L"Hello", 6, &errors);
  QCOMPARE(buff, "Hello");
  QCOMPARE(converted, 6);
  QVERIFY(!errors);
}

void IArchString_Test::convertStringWCToMB_noBuffer()
{
  SampleIArchString as;
  bool errors;
  auto converted = as.convStringWCToMB(nullptr, L"Hello", 6, &errors);
  QCOMPARE(converted, 6);
  QVERIFY(!errors);
}

void IArchString_Test::convertStringMBToWC()
{
  SampleIArchString as;
  wchar_t buff[20];
  bool errors;
  auto converted = as.convStringMBToWC(buff, "Hello", 6, &errors);

  auto actual = QString::fromStdWString(buff);
  auto expected = QString::fromStdWString(L"Hello");

  QCOMPARE(actual, expected);
  QCOMPARE(converted, 6);
  QVERIFY(!errors);
}

QTEST_MAIN(IArchString_Test)
