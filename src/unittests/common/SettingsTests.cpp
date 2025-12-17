/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsTests.h"

#include <QFile>
#include <QSignalSpy>

void SettingsTests::initTestCase()
{
  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();
}

void SettingsTests::setSettingsFile()
{
  Settings::setSettingsFile(m_settingsFile);
}

void SettingsTests::setStateFile()
{
  Settings::setStateFile(m_stateFile);
}

void SettingsTests::settingsFile()
{
  QVERIFY(Settings::settingsFile().endsWith(m_settingsFile));
}

void SettingsTests::settingsPath()
{
  QVERIFY(Settings::settingsPath().endsWith(m_settingsPath));
}

void SettingsTests::tlsDir()
{
  QVERIFY(Settings::tlsDir().endsWith(m_expectedTlsDir));
}

void SettingsTests::tlsTrustedServersDb()
{
  QVERIFY(Settings::tlsTrustedServersDb().endsWith(m_expectedTlsServerDB));
}

void SettingsTests::tlsTrustedClientsDb()
{
  QVERIFY(Settings::tlsTrustedClientsDb().endsWith(m_expectedTlsClientDB));
}

void SettingsTests::checkValidSettings()
{
  QSignalSpy spy(Settings::instance(), &Settings::settingsChanged);
  QVERIFY(spy.isValid());

  const auto keysToCheck = QRegularExpression(QLatin1String("[^%1]").arg(Settings::Core::ScreenName));
  const auto validKeys = Settings::validKeys().filter(keysToCheck);
  for (const auto &setting : validKeys) {
    const auto value = Settings::value(setting).toString();
    QCOMPARE(Settings::defaultValue(setting).toString(), value);

    Settings::setValue(setting, "NEW_VALUE");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<QString>(spy.first().at(0)), setting);
    QCOMPARE(Settings::value(setting).toString(), "NEW_VALUE");

    Settings::setValue(setting, QVariant());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(Settings::value(setting).toString(), value);

    // Reset the spy for the next loop
    spy.clear();
    QCOMPARE(spy.count(), 0);
  }
}

void SettingsTests::checkCleanScreenName()
{
  const auto input = QStringLiteral("--!_ _-S@c#r$e%e^&*(n)= +Name\n[1]2|3?4--5>6<,7`~/8*90\\.lan--..    ..");
  const auto expected = QStringLiteral("Screen_Name_1234--567890.lan");

  Settings::setValue(Settings::Core::ScreenName, input);

  QCOMPARE(Settings::value(Settings::Core::ScreenName).toString(), expected);
}

void SettingsTests::checkCleanScreenName_LongName()
{
  QString input;
  input.fill('f', 300);
  input.prepend('.');

  QString expected;
  expected.fill('f', 255);

  Settings::setValue(Settings::Core::ScreenName, input);

  QCOMPARE(Settings::value(Settings::Core::ScreenName).toString(), expected);
}

void SettingsTests::checkLogLevels_Valid()
{
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("Fatal")), 0);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("erRor")), 1);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("wArning")), 2);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("notE")), 3);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("info")), 4);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("deBug")), 5);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("debuG1")), 6);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("dEbug2")), 7);
}

void SettingsTests::checkLogLevels_Invalid()
{
  QCOMPARE(Settings::logLevelToInt(QString()), 4);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("INVALID")), 4);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("debug3")), 4);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("infomatic")), 4);
  QCOMPARE(Settings::logLevelToInt(QStringLiteral("warn")), 4);
}

QTEST_MAIN(SettingsTests)
