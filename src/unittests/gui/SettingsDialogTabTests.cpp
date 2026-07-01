/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialogTabTests.h"

#include "common/Settings.h"
#include "gui/config/ServerConfig.h"
#include "gui/dialogs/SettingsDialog.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTabWidget>

void SettingsDialogTabTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists()) {
    oldSettings.remove();
  }

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
}

void SettingsDialogTabTests::tab_switch_preserves_selection()
{
  ServerConfig serverConfig;
  SettingsDialog dialog(nullptr, serverConfig);
  dialog.setAttribute(Qt::WA_DontShowOnScreen);

  auto *tabWidget = dialog.findChild<QTabWidget *>();
  QVERIFY(tabWidget != nullptr);
  QVERIFY(tabWidget->count() > 1);

  for (int target = 0; target < tabWidget->count(); ++target) {
    const int previous = tabWidget->currentIndex();
    QSignalSpy spy(tabWidget, &QTabWidget::currentChanged);
    tabWidget->setCurrentIndex(target);
    QCOMPARE(spy.count(), previous == target ? 0 : 1);
    QCOMPARE(tabWidget->currentIndex(), target);
    QCOMPARE(tabWidget->currentWidget(), tabWidget->widget(target));
  }
}

void SettingsDialogTabTests::updateDialogHeight_preserves_active_tab()
{
  ServerConfig serverConfig;
  SettingsDialog dialog(nullptr, serverConfig);
  dialog.setAttribute(Qt::WA_DontShowOnScreen);

  auto *tabWidget = dialog.findChild<QTabWidget *>();
  QVERIFY(tabWidget != nullptr);
  QVERIFY(tabWidget->count() > 1);

  for (int target = 0; target < tabWidget->count(); ++target) {
    tabWidget->setCurrentIndex(target);
    dialog.updateDialogHeight();
    QCOMPARE(tabWidget->currentIndex(), target);
    QCOMPARE(tabWidget->currentWidget(), tabWidget->widget(target));
    QVERIFY(dialog.height() > 0);
  }
}

QTEST_MAIN(SettingsDialogTabTests)
