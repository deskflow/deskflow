/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerAppTests.h"

#include "deskflow/ServerApp.h"
#include "deskflow/ServerArgs.h"

void ServerAppTests::section()
{
  ServerApp app(nullptr);
  QVERIFY(!app.args().m_config);
  QCOMPARE(app.configSection(), "server");
}

QTEST_MAIN(ServerAppTests)
