/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenTests.h"

#include "common/Settings.h"
#include "gui/config/Screen.h"

void ScreenTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
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

void ScreenTests::span()
{
  Screen screen("stub");
  QCOMPARE(screen.width(), 1);
  QCOMPARE(screen.height(), 1);

  // span is clamped to at least one cell
  screen.setWidth(0);
  screen.setHeight(0);
  QCOMPARE(screen.width(), 1);
  QCOMPARE(screen.height(), 1);

  screen.setWidth(3);
  screen.setHeight(2);
  QCOMPARE(screen.width(), 3);
  QCOMPARE(screen.height(), 2);
  QVERIFY(!(screen == Screen("stub")));

  screen.saveSettings(Settings::proxy());
  Screen loaded;
  loaded.loadSettings(Settings::proxy());
  QCOMPARE(loaded.width(), 3);
  QCOMPARE(loaded.height(), 2);
}

QTEST_MAIN(ScreenTests)
