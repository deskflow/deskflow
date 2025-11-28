/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConnectionTests.h"

#include "gui/config/ServerConfig.h"
#include "gui/core/ServerConnection.h"
#include <common/Settings.h>

#include <QSignalSpy>

using namespace deskflow::gui;

class FullServerConfig : public ServerConfig
{
public:
  bool isFull() const override
  {
    return true;
  }
};

class ScreenExistsServerConfig : public ServerConfig
{
public:
  bool screenExists(const QString &screenName) const override
  {
    return true;
  }
};

void ServerConnectionTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
}

void ServerConnectionTests::handleLogLine_newClient_shouldShowPrompt()
{
  ServerConfig m_serverConfig;
  ServerConnection serverConnection(nullptr, m_serverConfig);

  QString clientName = "test client";

  QSignalSpy spy(&serverConnection, &ServerConnection::requestNewClientPrompt);
  QVERIFY(spy.isValid());

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
  QCOMPARE(spy.count(), 1);
}

void ServerConnectionTests::handleLogLine_ignoredClient_shouldNotShowPrompt()
{
  ServerConfig m_serverConfig;
  ServerConnection serverConnection(nullptr, m_serverConfig);
  QString clientName = "test client";
  serverConnection.handleNewClientResult(clientName, false);

  QSignalSpy spy(&serverConnection, &ServerConnection::requestNewClientPrompt);
  QVERIFY(spy.isValid());

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
  QCOMPARE(spy.count(), 0);
}

void ServerConnectionTests::handleLogLine_serverConfigFull_shouldNotShowPrompt()
{
  FullServerConfig m_serverConfig;
  ServerConnection serverConnection(nullptr, m_serverConfig);

  QSignalSpy spy(&serverConnection, &ServerConnection::requestNewClientPrompt);
  QVERIFY(spy.isValid());

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
  QCOMPARE(spy.count(), 0);
}

void ServerConnectionTests::handleLogLine_screenExists_shouldNotShowPrompt()
{
  ScreenExistsServerConfig m_serverConfig;
  ServerConnection serverConnection(nullptr, m_serverConfig);

  QSignalSpy spy(&serverConnection, &ServerConnection::requestNewClientPrompt);
  QVERIFY(spy.isValid());

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
  QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(ServerConnectionTests)
