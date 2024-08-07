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

#include "gui/CoreProcess.h"
#include "gui/IAppConfig.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <qglobal.h>

using namespace synergy::gui;
using ::testing::NiceMock;
using ::testing::ReturnRef;

class MockAppConfig : public IAppConfig {
public:
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

// TODO: stub out QProcess and IPC client
TEST(CoreProcessTests, testCoreProcess) {
  NiceMock<MockAppConfig> appConfig;
  NiceMock<MockServerConfig> serverConfig;
  CoreProcess coreProcess(appConfig, serverConfig);
  coreProcess.setMode(CoreProcess::Mode::Server);
  const QString stub = "stub";
  ON_CALL(appConfig, screenName()).WillByDefault(ReturnRef(stub));
  ON_CALL(appConfig, networkInterface()).WillByDefault(ReturnRef(stub));

  coreProcess.start(ProcessMode::kDesktop);
}
