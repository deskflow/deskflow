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
  MOCK_METHOD(void, signalReady, (), (override));
  MOCK_METHOD(
      bool, scopeContains, (const QString &name, Scope scope),
      (const, override));
  MOCK_METHOD(
      QVariant, getFromScope,
      (const QString &name, const QVariant &defaultValue, Scope scope),
      (const, override));
  MOCK_METHOD(
      void, setInScope,
      (const QString &name, const QVariant &value, Scope scope), (override));
  MOCK_METHOD(Scope, activeScope, (), (const, override));
  MOCK_METHOD(void, setActiveScope, (Scope scope), (override));
  MOCK_METHOD(bool, isActiveScopeWritable, (), (const, override));
  MOCK_METHOD(QSettings *, activeSettings, (), (const, override));
  MOCK_METHOD(void, save, (), (override));
};

struct MockDeps : public AppConfig::Deps {
  MockDeps() {
    ON_CALL(*this, profileDir()).WillByDefault(Return("stub"));
    ON_CALL(*this, hostname()).WillByDefault(Return("stub"));
  }

  static std::shared_ptr<NiceMock<MockDeps>> makeNice() {
    return std::make_shared<NiceMock<MockDeps>>();
  }

  MOCK_METHOD(QString, profileDir, (), (const, override));
  MOCK_METHOD(QString, hostname, (), (const, override));
};

} // namespace

class AppConfigTests : public Test {};

TEST_F(AppConfigTests, ctor_byDefault_screenNameIsHostname) {
  NiceMock<MockScopes> scopes;
  auto deps = MockDeps::makeNice();
  ON_CALL(*deps, hostname()).WillByDefault(Return("test hostname"));

  AppConfig appConfig(scopes, deps);

  ASSERT_EQ(appConfig.screenName().toStdString(), "test hostname");
}

TEST_F(AppConfigTests, ctor_byDefault_getsFromScope) {
  NiceMock<MockScopes> scopes;
  auto deps = MockDeps::makeNice();

  ON_CALL(scopes, scopeContains(_, _)).WillByDefault(Return(true));
  ON_CALL(scopes, getFromScope(_, _, _))
      .WillByDefault(Return(QVariant("test screen")));
  EXPECT_CALL(scopes, getFromScope(_, _, _));

  AppConfig appConfig(scopes, deps);

  ASSERT_EQ(appConfig.screenName().toStdString(), "test screen");
}

TEST_F(AppConfigTests, commit_byDefault_setsToScope) {
  NiceMock<MockScopes> scopes;
  auto deps = MockDeps::makeNice();
  AppConfig appConfig(scopes, deps);

  ON_CALL(scopes, isActiveScopeWritable()).WillByDefault(Return(true));
  EXPECT_CALL(scopes, setInScope(_, _, _)).Times(AnyNumber());

  appConfig.commit();
}
