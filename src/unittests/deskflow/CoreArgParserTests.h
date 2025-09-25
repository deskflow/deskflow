/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/CoreArgParser.h"

#include <QTest>

class CoreArgParserTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  // Test are run in order top to bottom
  void interfaceLong();
  void interfaceShort();
  void portLong();
  void portShort();
  void nameLong();
  void nameShort();
  void logLevel();
  void logFile();
  void logFileWithSpace();
  void secure_false();
  void secure_true();
  void secure_0();
  void secure_1();
  void tlsCert();
  void preventSleep_true();
  void preventSleep_false();
  void preventSleep_1();
  void preventSleep_0();
  void restartLongOption_0();
  void restartLongOption_1();
  void restartLongOption_false();
  void restartLongOption_true();
  void restartShortOption_0();
  void restartShortOption_1();
  void restartShortOption_false();
  void restartShortOption_true();
  void hookOptions_false();
  void hookOptions_true();
  void server_peerCheck_false();
  void server_peerCheck_true();
  void server_setConfig();
  void client_yscroll();
  void client_languageSync_true();
  void client_languageSync_false();
  void client_languageSync_1();
  void client_languageSync_0();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPath);
};
