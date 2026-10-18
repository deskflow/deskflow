/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "common/Settings.h"
#include "gui/dialogs/ServerConfigDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QTemporaryDir>
#include <QTest>

class ServerConfigDialogTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void compatibilityAndNavigation_saveReloadReset();

private:
  Log m_log;
  QTemporaryDir m_settings;
};

void ServerConfigDialogTests::initTestCase()
{
  QVERIFY(m_settings.isValid());
  Settings::setSettingsFile(m_settings.filePath("Deskflow.conf"));
  Settings::setStateFile(m_settings.filePath("Deskflow.state"));
  Settings::setValue(Settings::Core::ComputerName, "test-server");
}

void ServerConfigDialogTests::compatibilityAndNavigation_saveReloadReset()
{
  Settings::setValue(Settings::Server::Protocol, "synergy");
  Settings::setValue(Settings::Server::DefaultLockToComputerState, true);
  Settings::setValue(Settings::Server::ExternalConfig, false);
  ServerConfig config;
  {
    ServerConfigDialog dialog(nullptr, config);
    auto *compatibility = dialog.findChild<QCheckBox *>("cbMacOptionsCompatibility");
    auto *navigation = dialog.findChild<QCheckBox *>("cbMacNavigationGestures");
    auto *first = dialog.findChild<QComboBox *>("comboMacNavigationAction1");
    auto *second = dialog.findChild<QComboBox *>("comboMacNavigationAction2");
    QVERIFY(compatibility && navigation && first && second);
    QVERIFY(!compatibility->isChecked());
    QVERIFY(!navigation->isChecked());
    compatibility->setChecked(true);
    navigation->setChecked(true);
    first->setCurrentIndex(2);  // Up
    second->setCurrentIndex(3); // Down
    QVERIFY(dialog.isGeneralConfigModified());
    dialog.save();
  }
  QVERIFY(Settings::value(Settings::Server::MacOptionsCompatibilityEnabled).toBool());
  QVERIFY(Settings::value(Settings::Server::MacNavigationGesturesEnabled).toBool());
  QCOMPARE(Settings::value(Settings::Server::MacNavigationGestureAction1).toInt(), 3);
  QCOMPARE(Settings::value(Settings::Server::MacNavigationGestureAction2).toInt(), 4);
  QCOMPARE(Settings::value(Settings::Server::Protocol).toString(), QString("synergy"));
  QVERIFY(Settings::value(Settings::Server::DefaultLockToComputerState).toBool());
  {
    ServerConfigDialog dialog(nullptr, config);
    auto *compatibility = dialog.findChild<QCheckBox *>("cbMacOptionsCompatibility");
    auto *navigation = dialog.findChild<QCheckBox *>("cbMacNavigationGestures");
    auto *first = dialog.findChild<QComboBox *>("comboMacNavigationAction1");
    auto *second = dialog.findChild<QComboBox *>("comboMacNavigationAction2");
    QVERIFY(compatibility && navigation && first && second);
    QVERIFY(compatibility->isChecked());
    QVERIFY(navigation->isChecked());
    QCOMPARE(first->currentIndex(), 2);
    QCOMPARE(second->currentIndex(), 3);
    compatibility->setChecked(false);
    navigation->setChecked(false);
    dialog.resetFromSettings();
    QVERIFY(compatibility->isChecked());
    QVERIFY(navigation->isChecked());
    dialog.restoreFromDefaults();
    QVERIFY(!compatibility->isChecked());
    QVERIFY(!navigation->isChecked());
    QCOMPARE(first->currentIndex(), 0);
    QCOMPARE(second->currentIndex(), 1);
    QVERIFY(Settings::value(Settings::Server::MacOptionsCompatibilityEnabled).toBool());
  }
}

QTEST_MAIN(ServerConfigDialogTests)
#include "ServerConfigDialogTests.moc"
