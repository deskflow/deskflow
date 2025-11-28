/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientConnectionTests.h"

#include "gui/core/ClientConnection.h"
#include <common/Settings.h>

#include <QSignalSpy>

using namespace deskflow::gui;

void ClientConnectionTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
}

void ClientConnectionTests::handleLogLine_alreadyConnected_showError()
{
  ClientConnection clientConnection(nullptr);
  const auto serverName = QStringLiteral("test server");
  Settings::setValue(Settings::Client::RemoteHost, serverName);

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine(
      "failed to connect to server\n"
      "server already has a connected client with our name"
  );

  QCOMPARE(spy.count(), 1);
}

void ClientConnectionTests::handleLogLine_withHostname_showError()
{
  ClientConnection clientConnection(nullptr);
  const auto serverName = QStringLiteral("test server");
  Settings::setValue(Settings::Client::RemoteHost, serverName);

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine("failed to connect to server");

  QCOMPARE(spy.count(), 1);
}

void ClientConnectionTests::handleLogLine_withIpAddress_showError()
{
  ClientConnection clientConnection(nullptr);
  const auto serverName = QStringLiteral("1.1.1.1");
  Settings::setValue(Settings::Client::RemoteHost, serverName);

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine("failed to connect to server");

  QCOMPARE(spy.count(), 1);
}

void ClientConnectionTests::handleLogLine_serverRefusedClient_shouldNotShowError()
{
  ClientConnection clientConnection(nullptr);

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine(
      "failed to connect to server\n"
      "server refused client with our name"
  );

  QCOMPARE(spy.count(), 0);
}

void ClientConnectionTests::handleLogLine_connected_shouldPreventFutureError()
{
  ClientConnection clientConnection(nullptr);
  clientConnection.handleLogLine("connected to server");

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine("failed to connect to server");

  QCOMPARE(spy.count(), 0);
}

void ClientConnectionTests::handleLogLine_connectToggled_showAfterDisconnect()
{
  ClientConnection clientConnection(nullptr);
  clientConnection.handleLogLine("connected to server");

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine("failed to connect to server");
  clientConnection.handleLogLine("disconnected from server");
  clientConnection.handleLogLine("failed to connect to server");

  QCOMPARE(spy.count(), 1);
}

void ClientConnectionTests::handleLogLine_otherMessage_shouldNotShowError()
{
  ClientConnection clientConnection(nullptr);

  QSignalSpy spy(&clientConnection, &ClientConnection::requestShowError);
  QVERIFY(spy.isValid());

  clientConnection.handleLogLine("hello world");

  QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(ClientConnectionTests)
