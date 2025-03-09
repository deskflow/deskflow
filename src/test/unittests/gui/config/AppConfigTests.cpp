/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/config/AppConfig.h"

#include "gui/proxy/QSettingsProxy.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace deskflow::gui::proxy;

namespace {

class ConfigScopesMock : public deskflow::gui::IConfigScopes
{
  using QSettingsProxy = deskflow::gui::proxy::QSettingsProxy;

public:
  MOCK_METHOD(void, signalReady, (), (override));
  MOCK_METHOD(bool, scopeContains, (const QString &name, Scope scope), (const, override));
  MOCK_METHOD(
      QVariant, getFromScope, (const QString &name, const QVariant &defaultValue, Scope scope), (const, override)
  );
  MOCK_METHOD(void, setInScope, (const QString &name, const QVariant &value, Scope scope), (override));
  MOCK_METHOD(Scope, activeScope, (), (const, override));
  MOCK_METHOD(void, setActiveScope, (Scope scope), (override));
  MOCK_METHOD(bool, isActiveScopeWritable, (), (const, override));
  MOCK_METHOD(const QSettingsProxy &, activeSettings, (), (const, override));
  MOCK_METHOD(QSettingsProxy &, activeSettings, (), (override));
  MOCK_METHOD(void, save, (bool), (override));
  MOCK_METHOD(QString, activeFilePath, (), (const, override));
};

struct DepsMock : public AppConfig::Deps
{
  DepsMock()
  {
  }

  static std::shared_ptr<NiceMock<DepsMock>> makeNice()
  {
    return std::make_shared<NiceMock<DepsMock>>();
  }
};

} // namespace

class AppConfigTests : public Test
{
};

TEST_F(AppConfigTests, ctor_byDefault_getsFromScope)
{
  NiceMock<ConfigScopesMock> scopes;
  auto deps = DepsMock::makeNice();

  ON_CALL(scopes, scopeContains(_, _)).WillByDefault(Return(true));
  ON_CALL(scopes, getFromScope(_, _, _)).WillByDefault(Return(QVariant("test screen")));
  EXPECT_CALL(scopes, getFromScope(_, _, _)).Times(AnyNumber());

  AppConfig appConfig(scopes, deps);
}

TEST_F(AppConfigTests, commit_byDefault_setsToScope)
{
  NiceMock<ConfigScopesMock> scopes;
  auto deps = DepsMock::makeNice();
  AppConfig appConfig(scopes, deps);

  ON_CALL(scopes, isActiveScopeWritable()).WillByDefault(Return(true));
  EXPECT_CALL(scopes, setInScope(_, _, _)).Times(AnyNumber());

  appConfig.commit();
}
