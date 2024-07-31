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

#include "gui/AppConfig.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace {

class MockScopes : public synergy::gui::IConfigScopes {
public:
  MOCK_METHOD(
      void, registerReceiver, (synergy::gui::CommonConfig * receiver),
      (override));
  MOCK_METHOD(void, loadAll, (), (override));
  MOCK_METHOD(
      bool, hasSetting, (const QString &name, Scope scope), (const, override));
  MOCK_METHOD(
      QVariant, loadSetting,
      (const QString &name, const QVariant &defaultValue, Scope scope),
      (const, override));
  MOCK_METHOD(
      void, setSetting,
      (const QString &name, const QVariant &value, Scope scope), (override));
  MOCK_METHOD(Scope, getScope, (), (const, override));
  MOCK_METHOD(void, setScope, (Scope scope), (override));
  MOCK_METHOD(bool, isWritable, (), (const, override));
  MOCK_METHOD(QSettings *, currentSettings, (), (const, override));
  MOCK_METHOD(void, saveAll, (), (override));
};

struct MockDeps : public AppConfig::Deps {
  NiceMock<MockScopes> m_scopes;

  MockDeps() {
    ON_CALL(*this, profileDir()).WillByDefault(Return("stub"));
    ON_CALL(*this, scopes()).WillByDefault(ReturnRef(m_scopes));
    ON_CALL(*this, hostname()).WillByDefault(Return("stub"));
  }

  MOCK_METHOD(QString, profileDir, (), (const, override));
  MOCK_METHOD(synergy::gui::IConfigScopes &, scopes, (), (override));
  MOCK_METHOD(QString, hostname, (), (const, override));
};

} // namespace

class AppConfigTests : public Test {};

TEST_F(AppConfigTests, ctor_byDefault_screenNameIsHostname) {
  NiceMock<MockDeps> deps;
  ON_CALL(deps, hostname()).WillByDefault(Return("test"));

  AppConfig appConfig(deps);

  ASSERT_EQ(appConfig.screenName().toStdString(), "test");
}

TEST_F(AppConfigTests, loadAllScopes_byDefault_callsScopesLoadAll) {
  NiceMock<MockDeps> deps;
  AppConfig appConfig(deps);

  EXPECT_CALL(deps.m_scopes, loadAll());

  appConfig.loadAllScopes();
}

TEST_F(AppConfigTests, loadSettings_byDefault_callsScopesLoadSetting) {
  NiceMock<MockDeps> deps;
  AppConfig appConfig(deps);

  ON_CALL(deps.m_scopes, hasSetting(_, _)).WillByDefault(Return(true));
  ON_CALL(deps.m_scopes, loadSetting(_, _, _))
      .WillByDefault(Return(QVariant("test")));
  EXPECT_CALL(deps.m_scopes, loadSetting(_, _, _)).Times(AnyNumber());

  appConfig.loadSettings();

  ASSERT_EQ(appConfig.screenName().toStdString(), "test");
}

TEST_F(AppConfigTests, saveSettings_byDefault_callsScopesSetSetting) {
  NiceMock<MockDeps> deps;
  AppConfig appConfig(deps);

  ON_CALL(deps.m_scopes, isWritable()).WillByDefault(Return(true));
  EXPECT_CALL(deps.m_scopes, setSetting(_, _, _)).Times(AnyNumber());

  appConfig.saveSettings();
}
