/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class LogTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  // Test are run in order top to bottom
  void printWithErrorValidOutput();
  void printTestPrintLevel();
  void printTestWithArgs();
  void printTestLogString();
  void printLevelToHigh();
  void printInfoWithFileAndLine();
  void printErrWithFileAndLine();

private:
  Arch m_arch;
  Log m_log;
};
