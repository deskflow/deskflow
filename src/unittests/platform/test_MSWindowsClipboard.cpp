/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QtTest>

#include "test_MSWindowsClipboard.h"

#include "../../lib/platform/MSWindowsClipboard.h"

void MSWindowsClipboard_Test::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboard_Test::cleanupTestCase()
{
  initTestCase();
}

void MSWindowsClipboard_Test::emptyUnusedClipboard()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.emptyUnowned());
}

void MSWindowsClipboard_Test::emptyOpenCalled()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboard_Test::emptySingleFormat()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  clipboard.add(MSWindowsClipboard::kText, m_testString);
  QVERIFY(clipboard.empty());
  QVERIFY(!clipboard.has(MSWindowsClipboard::kText));
}

void MSWindowsClipboard_Test::addValue()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  clipboard.add(IClipboard::kText, m_testString);
  QCOMPARE(clipboard.get(IClipboard::kText), m_testString);
}

void MSWindowsClipboard_Test::replaceValue()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  clipboard.add(IClipboard::kText, m_testString);
  clipboard.add(IClipboard::kText, m_testString2); // haha, just kidding.
  QCOMPARE(clipboard.get(IClipboard::kText), m_testString2);
}

void MSWindowsClipboard_Test::openTimeIsOne()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
}

void MSWindowsClipboard_Test::closeIsOpen()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  clipboard.close();
}

void MSWindowsClipboard_Test::getTimeOpenWithNoEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  // this behavior is different to that of Clipboard which only
  // returns the value passed into open(t) after empty() is called.
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboard_Test::getTimeOpenAndEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboard_Test::has_withFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::kText, m_testString);
  QVERIFY(clipboard.has(IClipboard::kText));
}

void MSWindowsClipboard_Test::has_withNoFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.get(IClipboard::kText), "");
}

void MSWindowsClipboard_Test::getNonEmptyText()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::kText, m_testString);
  QCOMPARE(clipboard.get(IClipboard::kText), m_testString);
}

void MSWindowsClipboard_Test::isOwnedByDeskflow()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.isOwnedByDeskflow());
}

QTEST_MAIN(MSWindowsClipboard_Test)
