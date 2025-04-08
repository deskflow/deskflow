/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_Logger.h"

#include <QSignalSpy>

using namespace deskflow::gui;

void Logger_Test::newLine()
{
  Logger logger;
  QSignalSpy spy(&logger, &Logger::newLine);
  qputenv("DESKFLOW_GUI_DEBUG", "true");
  logger.loadEnvVars();

  logger.handleMessage(QtDebugMsg, "stub", "test");

  QCOMPARE(spy.count(), 1);
  QVERIFY(qvariant_cast<QString>(spy.takeFirst().at(0)).contains("test"));
  qputenv("DESKFLOW_GUI_DEBUG", "");
}

void Logger_Test::noNewLine()
{
  Logger logger;
  bool newLineEmitted = false;
  QSignalSpy spy(&logger, &Logger::newLine);
  qputenv("DESKFLOW_GUI_DEBUG", "false");
  logger.loadEnvVars();
  logger.handleMessage(QtDebugMsg, "stub", "test");
  QCOMPARE(spy.count(), 0);
  QVERIFY(!newLineEmitted);
  qputenv("DESKFLOW_GUI_DEBUG", "");
}

QTEST_MAIN(Logger_Test)
