/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class ClientConnectionTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void initTestCase();
  void handleLogLine_alreadyConnected_showError();
  void handleLogLine_withHostname_showError();
  void handleLogLine_withIpAddress_showError();
  void handleLogLine_serverRefusedClient_shouldNotShowError();
  void handleLogLine_connected_shouldPreventFutureError();
  void handleLogLine_connectToggled_showAfterDisconnect();
  void handleLogLine_otherMessage_shouldNotShowError();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPath);
  inline static const QString m_stateFile = QStringLiteral("%1/Deskflow.state").arg(m_settingsPath);
};
