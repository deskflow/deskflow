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

#include "gui/config/Screen.h"

#include "gui/proxy/QSettingsProxy.h"
#include "shared/gui/TestQtCoreApp.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace synergy::gui::proxy;
using namespace testing;

class QSettingsProxyMock : public QSettingsProxy {
public:
  MOCK_METHOD(int, beginReadArray, (QAnyStringView prefix), (override));
  MOCK_METHOD(void, beginWriteArray, (QAnyStringView prefix), (override));
  MOCK_METHOD(void, setArrayIndex, (int i), (override));
  MOCK_METHOD(QVariant, value, (QAnyStringView key), (const, override));
  MOCK_METHOD(
      QVariant, value, (QAnyStringView key, const QVariant &defaultValue),
      (const, override));
  MOCK_METHOD(void, endArray, (), (override));
  MOCK_METHOD(
      void, setValue, (QAnyStringView key, const QVariant &value), (override));
  MOCK_METHOD(void, beginGroup, (QAnyStringView prefix), (override));
  MOCK_METHOD(void, endGroup, (), (override));
  MOCK_METHOD(void, remove, (QAnyStringView key), (override));
  MOCK_METHOD(bool, isWritable, (), (const, override));
  MOCK_METHOD(bool, contains, (QAnyStringView key), (const, override));
};

TEST(ScreenTests, loadSettings_whenHasSetting_readsArray) {
  TestQtCoreApp app;
  NiceMock<QSettingsProxyMock> settings;
  Screen screen;
  ON_CALL(settings, value(_)).WillByDefault(Return("stub"));

  EXPECT_CALL(settings, beginReadArray(_)).Times(4);

  screen.loadSettings(settings);
}

TEST(ScreenTests, saveSettings_whenNameIsSet_writesArray) {
  TestQtCoreApp app;
  NiceMock<QSettingsProxyMock> settings;
  Screen screen;
  screen.setName("stub");

  EXPECT_CALL(settings, beginWriteArray(_)).Times(4);

  screen.saveSettings(settings);
}
