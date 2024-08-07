/*
 * synergy -- mouse and keyboard sharing utility
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

#include "gui/config/IAppConfig.h"
#include "gui/core/CoreProcess.h"
#include "gui/ipc/IQIpcClient.h"
#include "gui/proxy/QProcessProxy.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace synergy::gui;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

class MockAppConfig : public IAppConfig {
public:
  MockAppConfig() {
    ON_CALL(*this, screenName()).WillByDefault(ReturnRef(m_stubName));
    ON_CALL(*this, networkInterface())
        .WillByDefault(ReturnRef(m_stubInterface));
    ON_CALL(*this, logLevelText()).WillByDefault(Return("stub log level"));
  }

  MOCK_METHOD(QString, tlsCertPath, (), (const, override));
  MOCK_METHOD(QString, tlsKeyLength, (), (const, override));
  MOCK_METHOD(bool, tlsEnabled, (), (const, override));
  MOCK_METHOD(ProcessMode, processMode, (), (const, override));
  MOCK_METHOD(ElevateMode, elevateMode, (), (const, override));
  MOCK_METHOD(QString, logLevelText, (), (const, override));
  MOCK_METHOD(const QString &, screenName, (), (const, override));
  MOCK_METHOD(bool, preventSleep, (), (const, override));
  MOCK_METHOD(bool, logToFile, (), (const, override));
  MOCK_METHOD(const QString &, logFilename, (), (const, override));
  MOCK_METHOD(QString, coreServerName, (), (const, override));
  MOCK_METHOD(QString, coreClientName, (), (const, override));
  MOCK_METHOD(bool, invertConnection, (), (const, override));
  MOCK_METHOD(void, persistLogDir, (), (const, override));
  MOCK_METHOD(QString, serialKey, (), (const, override));
  MOCK_METHOD(bool, languageSync, (), (const, override));
  MOCK_METHOD(bool, invertScrollDirection, (), (const, override));
  MOCK_METHOD(int, port, (), (const, override));
  MOCK_METHOD(bool, useExternalConfig, (), (const, override));
  MOCK_METHOD(const QString &, configFile, (), (const, override));
  MOCK_METHOD(const QString &, networkInterface, (), (const, override));

private:
  const QString m_stubName = "stub name";
  const QString m_stubInterface = "stub interface";
  const QString m_stubAddress = "stub address";
};

class MockServerConfig : public IServerConfig {
public:
  MOCK_METHOD(bool, isFull, (), (const, override));
  MOCK_METHOD(
      bool, screenExists, (const QString &screenName), (const, override));
  MOCK_METHOD(bool, save, (const QString &fileName), (const, override));
  MOCK_METHOD(void, save, (QFile & file), (const, override));
  MOCK_METHOD(bool, enableDragAndDrop, (), (const, override));
};

class MockQProcessProxy : public proxy::QProcessProxy {
public:
  operator bool() const override { return toBool(); }

  MockQProcessProxy() {
    ON_CALL(*this, toBool()).WillByDefault(Return(true));
    ON_CALL(*this, state())
        .WillByDefault(Return(QProcess::ProcessState::Running));
    ON_CALL(*this, waitForStarted()).WillByDefault(Return(true));
  }

  MOCK_METHOD(bool, toBool, (), (const));
  MOCK_METHOD(void, create, (), (override));
  MOCK_METHOD(
      void, start, (const QString &program, const QStringList &arguments),
      (override));
  MOCK_METHOD(bool, waitForStarted, (), (override));
  MOCK_METHOD(QProcess::ProcessState, state, (), (const, override));
  MOCK_METHOD(void, close, (), (override));
  MOCK_METHOD(void, reset, (), (override));
  MOCK_METHOD(QString, readAllStandardOutput, (), (override));
  MOCK_METHOD(QString, readAllStandardError, (), (override));
};

class MockQIpcClient : public ipc::IQIpcClient {
public:
  MockQIpcClient() {
    ON_CALL(*this, isConnected()).WillByDefault(Return(true));
  }

  MOCK_METHOD(void, sendHello, (), (const, override));
  MOCK_METHOD(
      void, sendCommand, (const QString &command, ElevateMode elevate),
      (const, override));
  MOCK_METHOD(void, connectToHost, (), (override));
  MOCK_METHOD(void, disconnectFromHost, (), (override));
  MOCK_METHOD(bool, isConnected, (), (const, override));
};

class MockDeps : public CoreProcess::Deps {
public:
  MockDeps() {
    ON_CALL(*this, process()).WillByDefault(ReturnRef(m_process));
    ON_CALL(*this, ipcClient()).WillByDefault(ReturnRef(m_ipcClient));
    ON_CALL(*this, appPath(_)).WillByDefault(Return("stub app path"));
    ON_CALL(*this, fileExists(_)).WillByDefault(Return(true));
    ON_CALL(*this, getProfileRoot()).WillByDefault(Return("stub profile"));
  }

  MOCK_METHOD(proxy::QProcessProxy &, process, (), (override));
  MOCK_METHOD(ipc::IQIpcClient &, ipcClient, (), (override));
  MOCK_METHOD(QString, appPath, (const QString &name), (const, override));
  MOCK_METHOD(bool, fileExists, (const QString &path), (const, override));
  MOCK_METHOD(QString, getProfileRoot, (), (const, override));

  NiceMock<MockQProcessProxy> m_process;
  NiceMock<MockQIpcClient> m_ipcClient;
};

class CoreProcessTests : public ::testing::Test {
public:
  CoreProcessTests() : m_coreProcess(m_appConfig, m_serverConfig, m_pDeps) {}

  NiceMock<MockAppConfig> m_appConfig;
  NiceMock<MockServerConfig> m_serverConfig;
  std::shared_ptr<NiceMock<MockDeps>> m_pDeps =
      std::make_shared<NiceMock<MockDeps>>();
  CoreProcess m_coreProcess;
};

TEST_F(CoreProcessTests, start_serverDesktop_callsProcessStart) {
  m_coreProcess.setMode(CoreProcess::Mode::Server);

  EXPECT_CALL(m_pDeps->m_process, start(_, _)).Times(1);

  m_coreProcess.start(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, start_serverService_callsSendCommand) {
  m_coreProcess.setMode(CoreProcess::Mode::Server);

  EXPECT_CALL(m_pDeps->m_ipcClient, sendCommand(_, _)).Times(1);

  m_coreProcess.start(ProcessMode::kService);
}

TEST_F(CoreProcessTests, start_clientDesktop_callsProcessStart) {
  m_coreProcess.setMode(CoreProcess::Mode::Client);
  m_coreProcess.setAddress("stub address");

  EXPECT_CALL(m_pDeps->m_process, start(_, _)).Times(1);

  m_coreProcess.start(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, start_clientService_callsSendCommand) {
  m_coreProcess.setMode(CoreProcess::Mode::Client);
  m_coreProcess.setAddress("stub address");

  EXPECT_CALL(m_pDeps->m_ipcClient, sendCommand(_, _)).Times(1);

  m_coreProcess.start(ProcessMode::kService);
}

TEST_F(CoreProcessTests, stop_serverDesktop_callsProcessClose) {
  m_coreProcess.setMode(CoreProcess::Mode::Server);
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_process, close()).Times(1);

  m_coreProcess.stop(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, stop_serverService_callsSendCommand) {
  m_coreProcess.setMode(CoreProcess::Mode::Server);
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_ipcClient, sendCommand(_, _)).Times(1);

  m_coreProcess.stop(ProcessMode::kService);
}

TEST_F(CoreProcessTests, stop_clientDesktop_callsProcessClose) {
  m_coreProcess.setMode(CoreProcess::Mode::Client);
  m_coreProcess.setAddress("stub address");
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_process, close()).Times(1);

  m_coreProcess.stop(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, stop_clientService_callsSendCommand) {
  m_coreProcess.setMode(CoreProcess::Mode::Client);
  m_coreProcess.setAddress("stub address");
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_ipcClient, sendCommand(_, _)).Times(1);

  m_coreProcess.stop(ProcessMode::kService);
}

TEST_F(CoreProcessTests, restart_serverDesktop_callsProcessStart) {
  ON_CALL(m_appConfig, processMode())
      .WillByDefault(Return(ProcessMode::kDesktop));
  m_coreProcess.setMode(CoreProcess::Mode::Server);
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_process, close()).Times(1);
  EXPECT_CALL(m_pDeps->m_process, start(_, _)).Times(1);

  m_coreProcess.restart();
}
