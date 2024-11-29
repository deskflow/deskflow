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

#pragma once

#include "gui/config/IAppConfig.h"

#include "gui/config/ElevateMode.h"

#include <gmock/gmock.h>

class AppConfigMock : public deskflow::gui::IAppConfig
{
  using ProcessMode = deskflow::gui::ProcessMode;

public:
  AppConfigMock()
  {
    ON_CALL(*this, screenName()).WillByDefault(testing::ReturnRef(m_stub));

    ON_CALL(*this, networkInterface()).WillByDefault(testing::ReturnRef(m_stub));

    ON_CALL(*this, logLevelText()).WillByDefault(testing::Return(m_stub));

    ON_CALL(*this, logFilename()).WillByDefault(testing::ReturnRef(m_stub));
  }

  //
  // Getters
  //

  MOCK_METHOD(deskflow::gui::IConfigScopes &, scopes, (), (const, override));
  MOCK_METHOD(QString, tlsCertPath, (), (const, override));
  MOCK_METHOD(int, tlsKeyLength, (), (const, override));
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
  MOCK_METHOD(bool, languageSync, (), (const, override));
  MOCK_METHOD(bool, invertScrollDirection, (), (const, override));
  MOCK_METHOD(int, port, (), (const, override));
  MOCK_METHOD(bool, useExternalConfig, (), (const, override));
  MOCK_METHOD(const QString &, configFile, (), (const, override));
  MOCK_METHOD(const QString &, networkInterface, (), (const, override));
  MOCK_METHOD(const QString &, serverHostname, (), (const, override));
  MOCK_METHOD(int, logLevel, (), (const, override));
  MOCK_METHOD(bool, autoHide, (), (const, override));
  MOCK_METHOD(bool, enableService, (), (const, override));
  MOCK_METHOD(bool, closeToTray, (), (const, override));
  MOCK_METHOD(bool, isActiveScopeSystem, (), (const, override));
  MOCK_METHOD(bool, isActiveScopeWritable, (), (const, override));
  MOCK_METHOD(bool, clientGroupChecked, (), (const, override));
  MOCK_METHOD(bool, requireClientCerts, (), (const, override));

  //
  // Setters
  //

  MOCK_METHOD(void, setLoadFromSystemScope, (bool loadFromSystemScope), (override));
  MOCK_METHOD(void, setScreenName, (const QString &screenName), (override));
  MOCK_METHOD(void, setPort, (int port), (override));
  MOCK_METHOD(void, setNetworkInterface, (const QString &networkInterface), (override));
  MOCK_METHOD(void, setLogLevel, (int logLevel), (override));
  MOCK_METHOD(void, setLogToFile, (bool logToFile), (override));
  MOCK_METHOD(void, setLogFilename, (const QString &logFilename), (override));
  MOCK_METHOD(void, setElevateMode, (ElevateMode elevateMode), (override));
  MOCK_METHOD(void, setAutoHide, (bool autoHide), (override));
  MOCK_METHOD(void, setPreventSleep, (bool preventSleep), (override));
  MOCK_METHOD(void, setTlsCertPath, (const QString &tlsCertPath), (override));
  MOCK_METHOD(void, setTlsKeyLength, (int tlsKeyLength), (override));
  MOCK_METHOD(void, setTlsEnabled, (bool tlsEnabled), (override));
  MOCK_METHOD(void, setLanguageSync, (bool languageSync), (override));
  MOCK_METHOD(void, setInvertScrollDirection, (bool invertScrollDirection), (override));
  MOCK_METHOD(void, setEnableService, (bool enableService), (override));
  MOCK_METHOD(void, setCloseToTray, (bool closeToTray), (override));
  MOCK_METHOD(void, setInvertConnection, (bool invertConnection), (override));
  MOCK_METHOD(void, setRequireClientCerts, (bool requireClientCerts), (override));

private:
  const QString m_stub = "stub";
};
