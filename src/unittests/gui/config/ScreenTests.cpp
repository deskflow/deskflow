/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenTests.h"

#include "../../../lib/common/Settings.h"
#include "../../../lib/gui/config/Screen.h"

void ScreenTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingFile(m_settingsFile);
}

void ScreenTests::basicFunctionality()
{
  Screen screen;
  QVERIFY(screen.isNull());

  screen.setName("stub");
  QVERIFY(!screen.isNull());

  screen.saveSettings(Settings::proxy());
  screen.loadSettings(Settings::proxy());
  QCOMPARE("stub", screen.name());
}

QTEST_MAIN(ScreenTests)
