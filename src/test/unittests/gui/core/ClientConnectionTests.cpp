/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/core/ClientConnection.h"

#include "shared/gui/mocks/AppConfigMock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class QWidget;

using testing::_;
using testing::NiceMock;
using namespace deskflow::gui;
using enum messages::ClientError;

namespace {

struct DepsMock : public ClientConnection::Deps
{
  MOCK_METHOD(
      void, showError, (QWidget * parent, messages::ClientError error, const QString &address), (const, override)
  );
};

} // namespace

class ClientConnectionTests : public testing::Test
{
public:
  ClientConnectionTests()
  {
    ON_CALL(m_appConfig, serverHostname()).WillByDefault(testing::ReturnRef(stub));
  }

  std::shared_ptr<DepsMock> m_pDeps = std::make_shared<NiceMock<DepsMock>>();
  NiceMock<AppConfigMock> m_appConfig;

private:
  const QString stub = "stub";
};

TEST_F(ClientConnectionTests, handleLogLine_alreadyConnected_showError)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);
  const QString serverName = "test server";
  ON_CALL(m_appConfig, serverHostname()).WillByDefault(testing::ReturnRef(serverName));

  EXPECT_CALL(*m_pDeps, showError(_, AlreadyConnected, serverName));

  clientConnection.handleLogLine("failed to connect to server\n"
                                 "server already has a connected client with our name");
}

TEST_F(ClientConnectionTests, handleLogLine_withHostname_showError)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);
  const QString serverName = "test-hostname";
  ON_CALL(m_appConfig, serverHostname()).WillByDefault(testing::ReturnRef(serverName));

  EXPECT_CALL(*m_pDeps, showError(_, HostnameError, serverName));

  clientConnection.handleLogLine("failed to connect to server");
}

TEST_F(ClientConnectionTests, handleLogLine_withIpAddress_showError)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);
  const QString serverName = "1.1.1.1";
  ON_CALL(m_appConfig, serverHostname()).WillByDefault(testing::ReturnRef(serverName));

  EXPECT_CALL(*m_pDeps, showError(_, GenericError, serverName));

  clientConnection.handleLogLine("failed to connect to server");
}

TEST_F(ClientConnectionTests, handleLogLine_messageShown_shouldNotShowAgain)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);

  clientConnection.handleLogLine("failed to connect to server");

  EXPECT_CALL(*m_pDeps, showError(_, _, _)).Times(0);

  clientConnection.handleLogLine("failed to connect to server");
}

TEST_F(ClientConnectionTests, handleLogLine_serverRefusedClient_shouldNotShowError)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);

  EXPECT_CALL(*m_pDeps, showError(_, _, _)).Times(0);

  clientConnection.handleLogLine("failed to connect to server\n"
                                 "server refused client with our name");
}

TEST_F(ClientConnectionTests, handleLogLine_connected_shouldPreventFutureError)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);
  clientConnection.handleLogLine("connected to server");

  EXPECT_CALL(*m_pDeps, showError(_, _, _)).Times(0);

  clientConnection.handleLogLine("failed to connect to server");
}

TEST_F(ClientConnectionTests, handleLogLine_otherMessage_shouldNotShowError)
{
  ClientConnection clientConnection(nullptr, m_appConfig, m_pDeps);

  EXPECT_CALL(*m_pDeps, showError(_, _, _)).Times(0);

  clientConnection.handleLogLine("hello world");
}
