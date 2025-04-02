/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IArchStringTests.h"

#include "../../lib/arch/IArchString.h"

void IArchStringTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void IArchStringTests::convertStringWCToMB_buffer()
{
  SampleIArchString as;
  char buff[20];
  bool errors;

  auto converted = as.convStringWCToMB(buff, L"Hello", 6, &errors);

  QCOMPARE(converted, 6);
  QCOMPARE(buff, "Hello");
  QVERIFY(!errors);
}

void IArchStringTests::convertStringWCToMB_noBuffer()
{
  SampleIArchString as;
  bool errors;

  auto converted = as.convStringWCToMB(nullptr, L"Hello", 6, &errors);

  QCOMPARE(converted, 6);
  QVERIFY(!errors);
}

void IArchStringTests::convertStringMBToWC()
{
  SampleIArchString as;
  wchar_t buff[20];
  bool errors;

  auto converted = as.convStringMBToWC(buff, "Hello", 6, &errors);
  QCOMPARE(converted, 6);

  auto actual = QString::fromStdWString(buff);
  auto expected = QString::fromStdWString(L"Hello");

  QCOMPARE(actual, expected);
  QVERIFY(!errors);
}

QTEST_MAIN(IArchStringTests)
