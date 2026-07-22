/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: MIT
 */

#include "CoreProcessTests.h"

#include "common/Settings.h"
#include "gui/config/ServerConfig.h"
#include "gui/core/CoreProcess.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>

using namespace deskflow::gui;

void CoreProcessTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
  Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
}

void CoreProcessTests::stop_terminatesDesktopChild()
{
  const auto childPath =
    QDir(QCoreApplication::applicationDirPath()).filePath(
      QStringLiteral("CoreProcessTestChild") + QLatin1String(QT_EXECUTABLE_SUFFIX));
  QVERIFY(QFile::exists(childPath));

  ServerConfig config;
  CoreProcess coreProcess(config, childPath);
  coreProcess.setMode(Settings::CoreMode::Client);

  QSignalSpy processStateChangedSpy(&coreProcess, &CoreProcess::processStateChanged);
  QVERIFY(processStateChangedSpy.isValid());

  coreProcess.start(Settings::ProcessMode::Desktop);
  QCOMPARE(coreProcess.processState(), deskflow::core::ProcessState::Started);

  coreProcess.stop(Settings::ProcessMode::Desktop);
  QTRY_COMPARE(coreProcess.processState(), deskflow::core::ProcessState::Stopped);
}

QTEST_MAIN(CoreProcessTests)
