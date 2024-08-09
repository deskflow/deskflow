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

#include "gui/core/CoreProcess.h"
#include "gui/dialogs/SettingsDialog.h"
#include "license/SerialKey.h"
#include "shared/gui/TestQtFullApp.h"
#include "shared/gui/mocks/AppConfigMock.h"
#include "shared/gui/mocks/ServerConfigMock.h"

#include <gtest/gtest.h>

using namespace testing;
using namespace synergy::license;
using namespace synergy::gui;

TEST(SettingsDialogTests, ctor_getsScreenName) {
  std::cout << "create app" << std::endl;
  TestQtFullApp app;
  std::cout << "create mocks" << std::endl;
  NiceMock<AppConfigMock> appConfig;
  NiceMock<ServerConfigMock> serverConfig;
  std::cout << "create license" << std::endl;
  SerialKey serialKey = SerialKey::invalid();
  serialKey.isValid = true;
  License license(serialKey);
  std::cout << "create core process" << std::endl;
  auto cpDeps = std::make_shared<CoreProcess::Deps>();
  CoreProcess coreProcess(appConfig, serverConfig, cpDeps);

  std::cout << "expect call" << std::endl;
  EXPECT_CALL(appConfig, screenName()).Times(1);

  std::cout << "create dialog" << std::endl;
  SettingsDialog settingsDialog(
      nullptr, appConfig, serverConfig, license, coreProcess);
}
