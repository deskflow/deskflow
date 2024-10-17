/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/config/ServerConfigDialogState.h"
#include "gui/core/ServerConnection.h"

#include "shared/gui/mocks/AppConfigMock.h"
#include "shared/gui/mocks/ServerConfigMock.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::NiceMock;
using namespace deskflow::gui;

class QWidget;

namespace {

struct DepsMock : public ServerConnection::Deps
{
  MOCK_METHOD(
      messages::NewClientPromptResult, showNewClientPrompt, (QWidget * parent, const QString &clientName),
      (const, override)
  );
};

} // namespace

class ServerConnectionTests : public testing::Test
{
public:
  std::shared_ptr<DepsMock> m_pDeps = std::make_shared<NiceMock<DepsMock>>();
  NiceMock<AppConfigMock> m_appConfig;
  NiceMock<ServerConfigMock> m_serverConfig;
  config::ServerConfigDialogState m_serverConfigDialogState;
};

TEST_F(ServerConnectionTests, handleLogLine_newClient_shouldShowPrompt)
{
  ServerConnection serverConnection(nullptr, m_appConfig, m_serverConfig, m_serverConfigDialogState, m_pDeps);

  QString clientName = "test client";
  EXPECT_CALL(*m_pDeps, showNewClientPrompt(_, clientName));

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
}

TEST_F(ServerConnectionTests, handleLogLine_ignoredClient_shouldNotShowPrompt)
{
  ServerConnection serverConnection(nullptr, m_appConfig, m_serverConfig, m_serverConfigDialogState, m_pDeps);
  ON_CALL(*m_pDeps, showNewClientPrompt(_, _)).WillByDefault(testing::Return(messages::NewClientPromptResult::Ignore));
  serverConnection.handleLogLine(R"(unrecognised client name "stub")");

  EXPECT_CALL(*m_pDeps, showNewClientPrompt(_, _)).Times(0);

  serverConnection.handleLogLine(R"(unrecognised client name "stub")");
}

TEST_F(ServerConnectionTests, handleLogLine_serverConfigFull_shouldNotShowPrompt)
{
  ServerConnection serverConnection(nullptr, m_appConfig, m_serverConfig, m_serverConfigDialogState, m_pDeps);
  ON_CALL(m_serverConfig, isFull()).WillByDefault(testing::Return(true));

  EXPECT_CALL(*m_pDeps, showNewClientPrompt(_, _)).Times(0);

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
}

TEST_F(ServerConnectionTests, handleLogLine_screenExists_shouldNotShowPrompt)
{
  ServerConnection serverConnection(nullptr, m_appConfig, m_serverConfig, m_serverConfigDialogState, m_pDeps);
  ON_CALL(m_serverConfig, screenExists(_)).WillByDefault(testing::Return(true));

  EXPECT_CALL(*m_pDeps, showNewClientPrompt(_, _)).Times(0);

  serverConnection.handleLogLine(R"(unrecognised client name "test client")");
}
