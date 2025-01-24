/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/config/ConfigScopes.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace deskflow::gui;
using namespace deskflow::gui::proxy;

namespace {

class QSettingsProxyMock : public QSettingsProxy
{
public:
  MOCK_METHOD(void, loadSystem, (), (override));
  MOCK_METHOD(void, loadUser, (), (override));
  MOCK_METHOD(QString, fileName, (), (const, override));
  MOCK_METHOD(void, sync, (), (override));
  MOCK_METHOD(bool, isWritable, (), (const, override));
  MOCK_METHOD(bool, contains, (const QString &), (const, override));
  MOCK_METHOD(QVariant, value, (const QString &), (const, override));
  MOCK_METHOD(QVariant, value, (const QString &, const QVariant &), (const, override));
  MOCK_METHOD(void, setValue, (const QString &, const QVariant &), (override));
};

struct DepsMock : public ConfigScopes::Deps
{
  DepsMock()
  {
    ON_CALL(*this, makeUserSettings()).WillByDefault(Return(m_pUserSettings));
    ON_CALL(*this, makeSystemSettings()).WillByDefault(Return(m_pSystemSettings));
  }

  MOCK_METHOD(std::shared_ptr<QSettingsProxy>, makeUserSettings, (), (override));
  MOCK_METHOD(std::shared_ptr<QSettingsProxy>, makeSystemSettings, (), (override));

  std::shared_ptr<QSettingsProxyMock> m_pUserSettings = std::make_shared<NiceMock<QSettingsProxyMock>>();
  std::shared_ptr<QSettingsProxyMock> m_pSystemSettings = std::make_shared<NiceMock<QSettingsProxyMock>>();
};

} // namespace

TEST(ConfigScopesTests, ctor_callsMakeUserSettings)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  EXPECT_CALL(*deps, makeUserSettings()).Times(1);

  ConfigScopes configScopes(deps);
}

TEST(ConfigScopesTests, ctor_callsMakeSystemSettings)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  EXPECT_CALL(*deps, makeSystemSettings()).Times(1);

  ConfigScopes configScopes(deps);
}

TEST(ConfigScopesTests, save_syncsBothScopes)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ConfigScopes configScopes(deps);

  EXPECT_CALL(*deps->m_pUserSettings, sync()).Times(1);
  EXPECT_CALL(*deps->m_pSystemSettings, sync()).Times(1);

  configScopes.save();
}

TEST(ConfigScopesTests, activeSettings_returnsUserSettingsByDefault)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ConfigScopes configScopes(deps);

  EXPECT_EQ(&configScopes.activeSettings(), deps->m_pUserSettings.get());
}

TEST(ConfigScopesTests, activeSettings_returnsSystemSettingsWhenSystemScope)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ConfigScopes configScopes(deps);
  configScopes.setActiveScope(ConfigScopes::Scope::System);

  EXPECT_EQ(&configScopes.activeSettings(), deps->m_pSystemSettings.get());
}

TEST(ConfigScopesTests, setActiveScope_setsCurrentScope)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ConfigScopes configScopes(deps);

  configScopes.setActiveScope(ConfigScopes::Scope::System);

  EXPECT_EQ(configScopes.activeScope(), ConfigScopes::Scope::System);
}

TEST(ConfigScopesTests, isActiveScopeWritable_returnsTrueWhenUserSettingsWritable)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ConfigScopes configScopes(deps);

  EXPECT_CALL(*deps->m_pUserSettings, isWritable()).WillOnce(Return(true));

  EXPECT_TRUE(configScopes.isActiveScopeWritable());
}

TEST(ConfigScopesTests, scopeContains_byDefault_returnsTrueWhenUserSettingsContainsKey)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ON_CALL(*deps->m_pUserSettings, contains(_)).WillByDefault(Return(true));

  ConfigScopes configScopes(deps);

  EXPECT_TRUE(configScopes.scopeContains("stub"));
}

TEST(ConfigScopesTests, scopeContains_userScope_returnsTrueWhenUserSettingsContainsKey)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ON_CALL(*deps->m_pUserSettings, contains(_)).WillByDefault(Return(true));

  ConfigScopes configScopes(deps);

  EXPECT_TRUE(configScopes.scopeContains("stub", ConfigScopes::Scope::User));
}

TEST(ConfigScopesTests, scopeContains_systemScope_returnsTrueWhenSystemSettingsContainsKey)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ON_CALL(*deps->m_pSystemSettings, contains(_)).WillByDefault(Return(true));

  ConfigScopes configScopes(deps);

  EXPECT_TRUE(configScopes.scopeContains("stub", ConfigScopes::Scope::System));
}

TEST(ConfigScopesTests, activeFilePath_returnsUserSettingsFileNameByDefault)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();
  ON_CALL(*deps->m_pUserSettings, fileName()).WillByDefault(Return("test"));

  ConfigScopes configScopes(deps);

  EXPECT_EQ(configScopes.activeFilePath(), "test");
}

TEST(ConfigScopesTests, getFromScope_byDefault_returnsValueFromActiveSettings)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();
  ON_CALL(*deps->m_pUserSettings, value(_, _)).WillByDefault(Return("test"));

  ConfigScopes configScopes(deps);

  EXPECT_EQ(configScopes.getFromScope("stub"), "test");
}

TEST(ConfigScopesTests, setInScope_byDefault_setsValueInActiveSettings)
{
  auto deps = std::make_shared<NiceMock<DepsMock>>();

  ConfigScopes configScopes(deps);

  EXPECT_CALL(*deps->m_pUserSettings, setValue(_, _)).Times(1);

  configScopes.setInScope("stub", "test");
}
