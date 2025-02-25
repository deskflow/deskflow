/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/config/IAppConfig.h"
#include "gui/core/CoreProcess.h"
#include "gui/proxy/QProcessProxy.h"
#include "shared/gui/mocks/AppConfigMock.h"
#include "shared/gui/mocks/ServerConfigMock.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace deskflow::gui;
using namespace testing;

namespace {

class QProcessProxyMock : public proxy::QProcessProxy
{
public:
  operator bool() const override
  {
    return toBool();
  }

  QProcessProxyMock()
  {
    ON_CALL(*this, toBool()).WillByDefault(Return(true));
    ON_CALL(*this, state()).WillByDefault(Return(QProcess::ProcessState::Running));
    ON_CALL(*this, waitForStarted()).WillByDefault(Return(true));
  }

  MOCK_METHOD(bool, toBool, (), (const));
  MOCK_METHOD(void, create, (), (override));
  MOCK_METHOD(void, start, (const QString &program, const QStringList &arguments), (override));
  MOCK_METHOD(bool, waitForStarted, (), (override));
  MOCK_METHOD(QProcess::ProcessState, state, (), (const, override));
  MOCK_METHOD(void, close, (), (override));
  MOCK_METHOD(QString, readAllStandardOutput, (), (override));
  MOCK_METHOD(QString, readAllStandardError, (), (override));
};

class DepsMock : public CoreProcess::Deps
{
public:
  DepsMock()
  {
    ON_CALL(*this, process()).WillByDefault(ReturnRef(m_process));
    ON_CALL(*this, appPath(_)).WillByDefault(Return("stub app path"));
    ON_CALL(*this, fileExists(_)).WillByDefault(Return(true));
    ON_CALL(*this, getProfileRoot()).WillByDefault(Return("stub profile"));
  }

  MOCK_METHOD(proxy::QProcessProxy &, process, (), (override));
  MOCK_METHOD(QString, appPath, (const QString &name), (const, override));
  MOCK_METHOD(bool, fileExists, (const QString &path), (const, override));
  MOCK_METHOD(QString, getProfileRoot, (), (const, override));

  NiceMock<QProcessProxyMock> m_process;
};

class CoreProcessTests : public Test
{
public:
  CoreProcessTests() : m_coreProcess(m_appConfig, m_serverConfig, m_pDeps)
  {
    ON_CALL(m_appConfig, useExternalConfig()).WillByDefault(testing::Return(true));
    ON_CALL(m_appConfig, configFile()).WillByDefault(testing::ReturnRef(m_configFile));
    ON_CALL(m_appConfig, processMode()).WillByDefault(Return(ProcessMode::kDesktop));
  }

  NiceMock<AppConfigMock> m_appConfig;
  NiceMock<ServerConfigMock> m_serverConfig;
  std::shared_ptr<NiceMock<DepsMock>> m_pDeps = std::make_shared<NiceMock<DepsMock>>();
  CoreProcess m_coreProcess;

private:
  const QString m_configFile = "tmp/deskflow-server.conf";
};

} // namespace

TEST_F(CoreProcessTests, start_serverDesktop_callsProcessStart)
{
  m_coreProcess.setMode(CoreProcess::Mode::Server);

  EXPECT_CALL(m_pDeps->m_process, start(_, _)).Times(1);

  m_coreProcess.start(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, start_clientDesktop_callsProcessStart)
{
  m_coreProcess.setMode(CoreProcess::Mode::Client);
  m_coreProcess.setAddress("stub address");

  EXPECT_CALL(m_pDeps->m_process, start(_, _)).Times(1);

  m_coreProcess.start(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, stop_serverDesktop_callsProcessClose)
{
  m_coreProcess.setMode(CoreProcess::Mode::Server);
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_process, close()).Times(1);

  m_coreProcess.stop(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, stop_clientDesktop_callsProcessClose)
{
  m_coreProcess.setMode(CoreProcess::Mode::Client);
  m_coreProcess.setAddress("stub address");
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_process, close()).Times(1);

  m_coreProcess.stop(ProcessMode::kDesktop);
}

TEST_F(CoreProcessTests, restart_serverDesktop_callsProcessStart)
{
  ON_CALL(m_appConfig, processMode()).WillByDefault(Return(ProcessMode::kDesktop));
  m_coreProcess.setMode(CoreProcess::Mode::Server);
  m_coreProcess.start();

  EXPECT_CALL(m_pDeps->m_process, close()).Times(1);
  EXPECT_CALL(m_pDeps->m_process, start(_, _)).Times(1);

  m_coreProcess.restart();
}
