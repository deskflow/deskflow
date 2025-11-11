/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LoggerTests.h"
#include "common/Settings.h"

#include "gui/Logger.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>

using namespace deskflow::gui;

void LoggerTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
}

void LoggerTests::newLine()
{
  QSignalSpy spy(Logger::instance(), &Logger::newLine);
  QVERIFY(spy.isValid());

  Settings::setValue(Settings::Log::GuiDebug, true);
  Logger::instance()->handleMessage(QtDebugMsg, "stub", "test");

  QCOMPARE(spy.count(), 1);
  QVERIFY(qvariant_cast<QString>(spy.takeFirst().at(0)).contains("test"));
  Settings::setValue(Settings::Log::GuiDebug, false);
}

void LoggerTests::noNewLine()
{
  bool newLineEmitted = false;

  QSignalSpy spy(Logger::instance(), &Logger::newLine);
  QVERIFY(spy.isValid());

  Settings::setValue(Settings::Log::GuiDebug, false);
  Logger::instance()->handleMessage(QtDebugMsg, "stub", "test");
  QCOMPARE(spy.count(), 0);
  QVERIFY(!newLineEmitted);
}

QTEST_MAIN(LoggerTests)
