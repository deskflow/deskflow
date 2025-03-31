/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class MSWindowsClipboardTests : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void initTestCase();
  void cleanupTestCase();
  void emptyUnusedClipboard();
  void emptyOpenCalled();
  void emptySingleFormat();
  void addValue();
  void replaceValue();
  void openTimeIsOne();
  void closeIsOpen();
  void getTimeOpenWithNoEmpty();
  void getTimeOpenAndEmpty();
  void has_withFormatAdded();
  void has_withNoFormatAdded();
  void getNonEmptyText();
  void isOwnedByDeskflow();

private:
  Arch m_arch;
  Log m_log;
  const std::string m_testString = "deskflow test string";
  const std::string m_testString2 = "Another String";
};
