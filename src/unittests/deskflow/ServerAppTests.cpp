/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerAppTests.h"

#include "deskflow/ServerApp.h"
#include "deskflow/ServerArgs.h"

#include <iostream>

class MockServerApp : public ServerApp
{
public:
  MockServerApp() : ServerApp(nullptr)
  {
  }
};

void ServerAppTests::version()
{
  MockServerApp app;
  QVERIFY(!app.args().m_config);

  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  app.version();

  std::cout.rdbuf(old);

  static QRegularExpression yearReg(".*[0-9]{4}-[0-9]{4} Deskflow Devs.*");
  auto result = yearReg.match(QString::fromLatin1(buffer.str()));
  QVERIFY(result.hasMatch());
}

QTEST_MAIN(ServerAppTests)
