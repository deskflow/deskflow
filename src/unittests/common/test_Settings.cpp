/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_Settings.h"
#include <QFile>
#include <QSignalSpy>

void Settings_Test::initTestCase()
{
  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();
}

void Settings_Test::setSettingsFile()
{
  Settings::setSettingFile(m_settingsFile);
}

void Settings_Test::settingsFile()
{
  QVERIFY(Settings::settingsFile().endsWith(m_settingsFile));
}

void Settings_Test::settingsPath()
{
  QVERIFY(Settings::settingsPath().endsWith(m_settingsPath));
}

void Settings_Test::tlsDir()
{
  QVERIFY(Settings::tlsDir().endsWith(m_expectedTlsDir));
}

void Settings_Test::tlsLocalDb()
{
  QVERIFY(Settings::tlsLocalDb().endsWith(m_expectedTlsLocalDB));
}

void Settings_Test::tlsTrustedServersDb()
{
  QVERIFY(Settings::tlsTrustedServersDb().endsWith(m_expectedTlsServerDB));
}

void Settings_Test::tlsTrustedClientsDb()
{
  QVERIFY(Settings::tlsTrustedClientsDb().endsWith(m_expectedTlsClientDB));
}

void Settings_Test::checkValidSettings()
{
  const auto validKeys = Settings::validKeys();
  for (const auto &setting : validKeys) {
    QSignalSpy spy(Settings::instance(), &Settings::settingsChanged);
    const auto value = Settings::value(setting).toString();
    QCOMPARE(Settings::defaultValue(setting).toString(), value);
    Settings::setValue(setting, "NEW_VALUE");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<QString>(spy.takeFirst().at(0)), setting);
    QCOMPARE(Settings::value(setting).toString(), "NEW_VALUE");
    Settings::setValue(setting, QVariant());
    QCOMPARE(Settings::value(setting).toString(), value);
  }
}

QTEST_MAIN(Settings_Test)
