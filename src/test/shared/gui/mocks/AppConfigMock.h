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

#pragma once

#include "gui/config/IAppConfig.h"

#include "gui/config/ElevateMode.h"

#include <gmock/gmock.h>

class AppConfigMock : public synergy::gui::IAppConfig {
  using ProcessMode = synergy::gui::ProcessMode;

public:
  AppConfigMock() {
    ON_CALL(*this, screenName()).WillByDefault(testing::ReturnRef(m_stubName));
    ON_CALL(*this, networkInterface())
        .WillByDefault(testing::ReturnRef(m_stubInterface));
    ON_CALL(*this, logLevelText())
        .WillByDefault(testing::Return("stub log level"));
  }

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
  MOCK_METHOD(QString, serialKey, (), (const, override));
  MOCK_METHOD(bool, languageSync, (), (const, override));
  MOCK_METHOD(bool, invertScrollDirection, (), (const, override));
  MOCK_METHOD(int, port, (), (const, override));
  MOCK_METHOD(bool, useExternalConfig, (), (const, override));
  MOCK_METHOD(const QString &, configFile, (), (const, override));
  MOCK_METHOD(const QString &, networkInterface, (), (const, override));
  MOCK_METHOD(const QString &, serverHostname, (), (const, override));

private:
  const QString m_stubName = "stub name";
  const QString m_stubInterface = "stub interface";
  const QString m_stubAddress = "stub address";
};
