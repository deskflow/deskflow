/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/config/Screen.h"

#include "gui/proxy/QSettingsProxy.h"
#include "shared/gui/TestQtCoreApp.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace deskflow::gui::proxy;
using namespace testing;

class QSettingsProxyMock : public QSettingsProxy
{
public:
  MOCK_METHOD(int, beginReadArray, (const QString &prefix), (override));
  MOCK_METHOD(void, beginWriteArray, (const QString &prefix), (override));
  MOCK_METHOD(void, setArrayIndex, (int i), (override));
  MOCK_METHOD(QVariant, value, (const QString &key), (const, override));
  MOCK_METHOD(QVariant, value, (const QString &key, const QVariant &defaultValue), (const, override));
  MOCK_METHOD(void, endArray, (), (override));
  MOCK_METHOD(void, setValue, (const QString &key, const QVariant &value), (override));
  MOCK_METHOD(void, beginGroup, (const QString &prefix), (override));
  MOCK_METHOD(void, endGroup, (), (override));
  MOCK_METHOD(void, remove, (const QString &key), (override));
  MOCK_METHOD(bool, isWritable, (), (const, override));
  MOCK_METHOD(bool, contains, (const QString &key), (const, override));
};

TEST(ScreenTests, loadSettings_whenHasSetting_readsArray)
{
  TestQtCoreApp app;
  NiceMock<QSettingsProxyMock> settings;
  Screen screen;
  ON_CALL(settings, value(_)).WillByDefault(Return("stub"));

  EXPECT_CALL(settings, beginReadArray(_)).Times(4);

  screen.loadSettings(settings);
}

TEST(ScreenTests, saveSettings_whenNameIsSet_writesArray)
{
  TestQtCoreApp app;
  NiceMock<QSettingsProxyMock> settings;
  Screen screen;
  screen.setName("stub");

  EXPECT_CALL(settings, beginWriteArray(_)).Times(4);

  screen.saveSettings(settings);
}
