/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "common/Settings.h"
#include "gui/MacSmoothModeController.h"
#include "gui/MacSmoothModeMenu.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/dialogs/SettingsDialog.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>

class MacPreferencesTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void init();
  void serverToggles_saveAndReloadInAdvanced();
  void cancelResetAndDefaults_preserveStoredValues();
  void clientRoleAndExternalMenuChanges_preservePendingEdits();
  void unavailableWorker_disablesOnlySmoothing();
  void layout_displaysMacTab();

private:
  Log m_log;
  QTemporaryDir m_settings;
};

namespace {
void writeFile(const QString &path, const QByteArray &contents)
{
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write(contents), contents.size());
}

QByteArray readFile(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return file.readAll();
}

QCheckBox *check(SettingsDialog &dialog, const char *name)
{
  return dialog.findChild<QCheckBox *>(name);
}

QPushButton *button(SettingsDialog &dialog, QDialogButtonBox::StandardButton kind)
{
  return dialog.findChild<QDialogButtonBox *>()->button(kind);
}
} // namespace

void MacPreferencesTests::initTestCase()
{
  QVERIFY(m_settings.isValid());
  Settings::setSettingsFile(m_settings.filePath("Deskflow.conf"));
  Settings::setStateFile(m_settings.filePath("Deskflow.state"));
  Settings::setValue(Settings::Core::ComputerName, "test-server");
}

void MacPreferencesTests::init()
{
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);
  Settings::setValue(Settings::Server::MacOptionsCompatibilityEnabled, false);
  Settings::setValue(Settings::Server::MacNavigationGesturesEnabled, false);
  Settings::setValue(Settings::Server::DefaultLockToComputerState, false);
}

void MacPreferencesTests::serverToggles_saveAndReloadInAdvanced()
{
  QTemporaryDir temp;
  const auto control = temp.filePath("control");
  const auto status = temp.filePath("status");
  writeFile(control, "auto\n");
  writeFile(status, "version=1\nmode=smooth\n");
  MacSmoothModeController controller(nullptr, control, status);
  MacSmoothModeMenu menu(nullptr, control, status);
  ServerConfig config;
  {
    SettingsDialog dialog(nullptr, config, &controller);
    auto *compatibility = check(dialog, "cbMacOptionsCompatibility");
    auto *navigation = check(dialog, "cbMacNavigationGestures");
    auto *lock = check(dialog, "cbMacLockToComputer");
    auto *automatic = check(dialog, "cbMacAutomaticSmoothing");
    QVERIFY(compatibility && navigation && lock && automatic);
    QVERIFY(compatibility->isEnabled() && navigation->isEnabled() && lock->isEnabled());
    compatibility->setChecked(true);
    navigation->setChecked(true);
    lock->setChecked(true);
    automatic->setChecked(false);
    // Edits are staged until Save.
    QCOMPARE(readFile(control), QByteArray("auto\n"));
    QVERIFY(!Settings::value(Settings::Server::MacOptionsCompatibilityEnabled).toBool());
    QVERIFY(button(dialog, QDialogButtonBox::Save)->isEnabled());
    button(dialog, QDialogButtonBox::Save)->click();
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
  }
  QVERIFY(Settings::value(Settings::Server::MacOptionsCompatibilityEnabled).toBool());
  QVERIFY(Settings::value(Settings::Server::MacNavigationGesturesEnabled).toBool());
  QVERIFY(Settings::value(Settings::Server::DefaultLockToComputerState).toBool());
  QCOMPARE(readFile(control), QByteArray("disabled\n"));
  menu.refresh();
  QVERIFY(!menu.findChild<QAction *>("smoothAutomatic")->isChecked());
  {
    ServerConfigDialog advanced(nullptr, config);
    QVERIFY(advanced.findChild<QCheckBox *>("cbMacOptionsCompatibility")->isChecked());
    QVERIFY(advanced.findChild<QCheckBox *>("cbMacNavigationGestures")->isChecked());
    QVERIFY(advanced.findChild<QCheckBox *>("cbDefaultLockToComputerState")->isChecked());
  }
  {
    SettingsDialog dialog(nullptr, config, &controller);
    QVERIFY(check(dialog, "cbMacOptionsCompatibility")->isChecked());
    QVERIFY(check(dialog, "cbMacNavigationGestures")->isChecked());
    QVERIFY(check(dialog, "cbMacLockToComputer")->isChecked());
    QVERIFY(!check(dialog, "cbMacAutomaticSmoothing")->isChecked());
  }
}

void MacPreferencesTests::cancelResetAndDefaults_preserveStoredValues()
{
  QTemporaryDir temp;
  const auto control = temp.filePath("control");
  const auto status = temp.filePath("status");
  writeFile(control, "auto\n");
  writeFile(status, "version=1\nmode=allowed\n");
  MacSmoothModeController controller(nullptr, control, status);
  ServerConfig config;
  SettingsDialog dialog(nullptr, config, &controller);
  check(dialog, "cbMacOptionsCompatibility")->setChecked(true);
  check(dialog, "cbMacAutomaticSmoothing")->setChecked(false);
  button(dialog, QDialogButtonBox::Reset)->click();
  QVERIFY(!check(dialog, "cbMacOptionsCompatibility")->isChecked());
  QVERIFY(check(dialog, "cbMacAutomaticSmoothing")->isChecked());
  button(dialog, QDialogButtonBox::RestoreDefaults)->click();
  QVERIFY(!check(dialog, "cbMacAutomaticSmoothing")->isChecked());
  QCOMPARE(readFile(control), QByteArray("auto\n"));
  dialog.reject();
  QVERIFY(!Settings::value(Settings::Server::MacOptionsCompatibilityEnabled).toBool());
  QCOMPARE(readFile(control), QByteArray("auto\n"));
}

void MacPreferencesTests::clientRoleAndExternalMenuChanges_preservePendingEdits()
{
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
  Settings::setValue(Settings::Server::MacOptionsCompatibilityEnabled, true);
  QTemporaryDir temp;
  const auto control = temp.filePath("control");
  const auto status = temp.filePath("status");
  const auto pause = "pause_until=" + QByteArray::number(QDateTime::currentSecsSinceEpoch() + 600) + '\n';
  writeFile(control, pause);
  writeFile(status, "version=1\nmode=airdrop\n");
  MacSmoothModeController controller(nullptr, control, status);
  MacSmoothModeMenu menu(nullptr, control, status);
  ServerConfig config;
  {
    SettingsDialog dialog(nullptr, config, &controller);
    QVERIFY(!check(dialog, "cbMacOptionsCompatibility")->isEnabled());
    QVERIFY(!check(dialog, "cbMacNavigationGestures")->isEnabled());
    QVERIFY(!check(dialog, "cbMacLockToComputer")->isEnabled());
    auto *automatic = check(dialog, "cbMacAutomaticSmoothing");
    QVERIFY(automatic->isEnabled());
    QVERIFY(automatic->isChecked());
    // Saving an unrelated preference must preserve an active AirDrop override.
    auto *showVersion = check(dialog, "cbShowVersion");
    showVersion->setChecked(!showVersion->isChecked());
    button(dialog, QDialogButtonBox::Save)->click();
    QCOMPARE(readFile(control), pause);
  }
  {
    SettingsDialog dialog(nullptr, config, &controller);
    auto *automatic = check(dialog, "cbMacAutomaticSmoothing");
    menu.findChild<QAction *>("smoothAutomatic")->trigger();
    controller.refresh();
    QVERIFY(!automatic->isChecked());
    automatic->setChecked(true);
    controller.refresh();
    QVERIFY(automatic->isChecked()); // A timer refresh cannot discard a pending edit.
    QCOMPARE(readFile(control), QByteArray("disabled\n"));
    dialog.reject();
  }
  QVERIFY(Settings::value(Settings::Server::MacOptionsCompatibilityEnabled).toBool());
  QCOMPARE(readFile(control), QByteArray("disabled\n"));
}

void MacPreferencesTests::unavailableWorker_disablesOnlySmoothing()
{
  QTemporaryDir temp;
  MacSmoothModeController controller(nullptr, temp.filePath("control"), temp.filePath("status"));
  ServerConfig config;
  SettingsDialog dialog(nullptr, config, &controller);
  QVERIFY(!check(dialog, "cbMacAutomaticSmoothing")->isEnabled());
  QVERIFY(check(dialog, "cbMacOptionsCompatibility")->isEnabled());
  QVERIFY(dialog.findChild<QLabel *>("lblMacSmoothStatus")->text().contains("offline"));
  QVERIFY(!QFile::exists(temp.filePath("control")));
}

void MacPreferencesTests::layout_displaysMacTab()
{
  QTemporaryDir temp;
  writeFile(temp.filePath("control"), "auto\n");
  writeFile(temp.filePath("status"), "version=1\nmode=allowed\n");
  MacSmoothModeController controller(nullptr, temp.filePath("control"), temp.filePath("status"));
  ServerConfig config;
  SettingsDialog dialog(nullptr, config, &controller);
  auto *tabs = dialog.findChild<QTabWidget *>("tabWidget");
  auto *page = dialog.findChild<QWidget *>("tabMacOS");
  QVERIFY(tabs->indexOf(page) >= 0);
  tabs->setCurrentWidget(page);
  dialog.show();
  QApplication::processEvents();
  QVERIFY(check(dialog, "cbMacAutomaticSmoothing")->isVisible());
  QVERIFY(tabs->tabBar()->sizeHint().width() <= tabs->width());
  QVERIFY(dialog.height() < 800);
  const auto screenshot = qEnvironmentVariable("DESKFLOW_TEST_SCREENSHOT");
  if (!screenshot.isEmpty())
    QVERIFY(dialog.grab().save(screenshot));
}

QTEST_MAIN(MacPreferencesTests)
#include "MacPreferencesTests.moc"
