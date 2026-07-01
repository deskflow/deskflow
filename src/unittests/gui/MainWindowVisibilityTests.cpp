/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MainWindowVisibilityTests.h"

#include "common/Constants.h"
#include "common/Settings.h"
#include "gui/MainWindow.h"

#if defined(Q_OS_MACOS)
#include "gui/OSXHelpers.h"
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFile>

void MainWindowVisibilityTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);

  const QDir testDir(QCoreApplication::applicationDirPath());
  const QString coreBinary = testDir.filePath(kCoreBinName);
  if (!QFile::exists(coreBinary)) {
    const QString bundledCore =
        testDir.filePath(QStringLiteral("../../../bin/Deskflow.app/Contents/MacOS/%1").arg(kCoreBinName));
    if (QFile::exists(bundledCore)) {
      QVERIFY(QFile::link(bundledCore, coreBinary));
    }
  }
  if (!QFile::exists(coreBinary)) {
    QSKIP("Core binary not available for MainWindow integration test");
  }
}

void MainWindowVisibilityTests::cleanupTestCase()
{
  const QDir testDir(QCoreApplication::applicationDirPath());
  const QString coreBinary = testDir.filePath(kCoreBinName);
  if (QFile::exists(coreBinary)) {
    QFile::remove(coreBinary);
  }
}

#if defined(Q_OS_MACOS)
void MainWindowVisibilityTests::macOS_dock_policy_round_trip()
{
  macOSSetDockVisible(false);
  QVERIFY(!macOSIsDockVisible());

  macOSSetDockVisible(true);
  QVERIFY(macOSIsDockVisible());

  macOSSetDockVisible(false);
  QVERIFY(!macOSIsDockVisible());
}

void MainWindowVisibilityTests::macOS_hide_removes_dock_icon()
{
  MainWindow window;
  window.show();
  QVERIFY(macOSIsDockVisible());

  window.hide();
  QVERIFY(!window.isVisible());
  QVERIFY(!macOSIsDockVisible());
}

void MainWindowVisibilityTests::macOS_show_restores_dock_icon()
{
  MainWindow window;
  window.show();
  window.hide();
  QVERIFY(!macOSIsDockVisible());

  window.show();
  QVERIFY(window.isVisible());
  QVERIFY(macOSIsDockVisible());
}
#endif

void MainWindowVisibilityTests::hide_marks_window_not_visible()
{
  MainWindow window;
  window.show();
  QVERIFY(window.isVisible());

  window.hide();
  QVERIFY(!window.isVisible());
}

void MainWindowVisibilityTests::show_after_hide_marks_window_visible()
{
  MainWindow window;
  window.show();
  window.hide();
  QVERIFY(!window.isVisible());

  window.show();
  QVERIFY(window.isVisible());
}

QTEST_MAIN(MainWindowVisibilityTests)
