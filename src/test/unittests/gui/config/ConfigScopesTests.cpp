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

#include "gui/config/ConfigScopes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace synergy::gui;
using namespace synergy::gui::proxy;

namespace {

class QSettingsProxyMock : public QSettingsProxy {
public:
  MOCK_METHOD(void, loadSystem, (), (override));
  MOCK_METHOD(void, loadUser, (), (override));
  MOCK_METHOD(QString, fileName, (), (const, override));
};

struct DepsMock : public ConfigScopes::Deps {
  DepsMock() {
    ON_CALL(*this, makeUserSettings()).WillByDefault([] {
      return std::make_shared<NiceMock<QSettingsProxyMock>>();
    });
    ON_CALL(*this, makeSystemSettings()).WillByDefault([] {
      return std::make_shared<NiceMock<QSettingsProxyMock>>();
    });
  }

  MOCK_METHOD(
      std::shared_ptr<QSettingsProxy>, makeUserSettings, (), (override));
  MOCK_METHOD(
      std::shared_ptr<QSettingsProxy>, makeSystemSettings, (), (override));
};
} // namespace

TEST(ConfigScopesTests, ctor_callsMakeUserSettings) {
  std::shared_ptr<NiceMock<DepsMock>> deps =
      std::make_shared<NiceMock<DepsMock>>();

  EXPECT_CALL(*deps, makeUserSettings()).Times(1);

  ConfigScopes configScopes(deps);
}

TEST(ConfigScopesTests, ctor_callsMakeSystemSettings) {
  std::shared_ptr<NiceMock<DepsMock>> deps =
      std::make_shared<NiceMock<DepsMock>>();

  EXPECT_CALL(*deps, makeSystemSettings()).Times(1);

  ConfigScopes configScopes(deps);
}
