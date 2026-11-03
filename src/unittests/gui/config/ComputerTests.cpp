/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerTests.h"

#include "common/Settings.h"
#include "gui/config/Computer.h"

#include <QDataStream>
#include <QIODevice>

void ComputerTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
}

void ComputerTests::basicFunctionality()
{
  Computer computer;
  QVERIFY(computer.isNull());

  computer.setName("stub");
  QVERIFY(!computer.isNull());
  QVERIFY(computer.autoModifierSwitch());
  computer.setAutoModifierSwitch(false);

  computer.saveSettings(Settings::proxy());
  computer.loadSettings(Settings::proxy());
  QCOMPARE("stub", computer.name());
  QVERIFY(!computer.autoModifierSwitch());

  QByteArray encoded;
  QDataStream output(&encoded, QIODevice::WriteOnly);
  output << computer;
  Computer decoded;
  QDataStream input(&encoded, QIODevice::ReadOnly);
  input >> decoded;
  QVERIFY(!decoded.autoModifierSwitch());
  QVERIFY(computer == decoded);
}

QTEST_MAIN(ComputerTests)
