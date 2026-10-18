/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MockEventQueue.h"
#include "RecordingPlatformComputer.h"
#include "base/Log.h"
#include "common/KeySequence.h"
#include "common/Settings.h"
#include "deskflow/Computer.h"

#include <QTemporaryDir>
#include <QTest>

class ComputerNavigationTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void defaults_generatePlatformNavigation();
  void customShortcut_preservesRecordedModifiers_data();
  void customShortcut_preservesRecordedModifiers();
  void customCtrlAltDelete_usesSecureAttention();
  void invalidCustomShortcut_isIgnored();

private:
  Log m_log;
  QTemporaryDir m_settings;
};

void ComputerNavigationTests::initTestCase()
{
  QVERIFY(m_settings.isValid());
  Settings::setSettingsFile(m_settings.filePath("Deskflow.conf"));
  Settings::setStateFile(m_settings.filePath("Deskflow.state"));
}

void ComputerNavigationTests::defaults_generatePlatformNavigation()
{
  MockEventQueue events;
  auto *platform = new RecordingPlatformComputer(&events);
  deskflow::Computer computer(platform, &events);
  computer.navigationGesture(NavigationActionSlot::Action1);
  computer.navigationGesture(NavigationActionSlot::Action2);
#ifdef Q_OS_MACOS
  QCOMPARE(platform->keys.size(), size_t(2));
  QCOMPARE(platform->keys[0].key, KeyID('['));
  QCOMPARE(platform->keys[1].key, KeyID(']'));
  QCOMPARE(platform->keys[0].mask, KeyModifierMask(KeyModifierSuper));
  QCOMPARE(platform->releases.size(), size_t(2));
#else
  QCOMPARE(platform->buttons.size(), size_t(4));
  QCOMPARE(platform->buttons[0].first, kButtonExtra0);
  QCOMPARE(platform->buttons[2].first, kButtonExtra1);
#endif
}

void ComputerNavigationTests::customShortcut_preservesRecordedModifiers_data()
{
  QTest::addColumn<int>("qtKey");
  QTest::addColumn<int>("qtModifier");
  QTest::addColumn<uint32_t>("expectedMask");
#ifdef Q_OS_MACOS
  // Qt's default macOS convention maps physical Command to ControlModifier
  // and physical Control to MetaModifier, as used by KeySequenceWidget.
  QTest::newRow("physical-command") << int(Qt::Key_Control) << int(Qt::ControlModifier) << uint32_t(KeyModifierSuper);
  QTest::newRow("physical-control") << int(Qt::Key_Meta) << int(Qt::MetaModifier) << uint32_t(KeyModifierControl);
#else
  QTest::newRow("control") << int(Qt::Key_Control) << int(Qt::ControlModifier) << uint32_t(KeyModifierControl);
  QTest::newRow("meta") << int(Qt::Key_Meta) << int(Qt::MetaModifier) << uint32_t(KeyModifierMeta);
#endif
  QTest::newRow("alt") << int(Qt::Key_Alt) << int(Qt::AltModifier) << uint32_t(KeyModifierAlt);
}

void ComputerNavigationTests::customShortcut_preservesRecordedModifiers()
{
  QFETCH(int, qtKey);
  QFETCH(int, qtModifier);
  QFETCH(uint32_t, expectedMask);
  KeySequence sequence;
  sequence.appendKey(qtKey, qtModifier);
  QVERIFY(sequence.appendKey(Qt::Key_BracketLeft, qtModifier));
  Settings::setValue(Settings::Client::NavigationGestureAction1, int(NavigationOutputAction::Keystroke));
  Settings::setValue(Settings::Client::NavigationGestureShortcut1, sequence.toString());
  MockEventQueue events;
  auto *platform = new RecordingPlatformComputer(&events);
  deskflow::Computer computer(platform, &events);
  computer.navigationGesture(NavigationActionSlot::Action1);
  QCOMPARE(platform->keys.size(), size_t(1));
  QCOMPARE(platform->keys[0].key, KeyID('['));
  QCOMPARE(platform->keys[0].mask, KeyModifierMask(expectedMask));
  QCOMPARE(platform->releases.size(), size_t(1));
}

void ComputerNavigationTests::customCtrlAltDelete_usesSecureAttention()
{
  Settings::setValue(Settings::Client::NavigationGestureAction1, int(NavigationOutputAction::Keystroke));
#ifdef Q_OS_MACOS
  Settings::setValue(Settings::Client::NavigationGestureShortcut1, "Meta+Alt+Delete");
#else
  Settings::setValue(Settings::Client::NavigationGestureShortcut1, "Control+Alt+Delete");
#endif
  MockEventQueue events;
  auto *platform = new RecordingPlatformComputer(&events);
  platform->handleSas = true;
  deskflow::Computer computer(platform, &events);
  computer.navigationGesture(NavigationActionSlot::Action1);
  QCOMPARE(platform->sasRequests, 1);
  QVERIFY(platform->keys.empty());
}

void ComputerNavigationTests::invalidCustomShortcut_isIgnored()
{
  Settings::setValue(Settings::Client::NavigationGestureAction1, int(NavigationOutputAction::Keystroke));
  Settings::setValue(Settings::Client::NavigationGestureShortcut1, "Control+NotAKey");
  MockEventQueue events;
  auto *platform = new RecordingPlatformComputer(&events);
  deskflow::Computer computer(platform, &events);
  computer.navigationGesture(NavigationActionSlot::Action1);
  QVERIFY(platform->keys.empty());
  QVERIFY(platform->releases.empty());
}

QTEST_GUILESS_MAIN(ComputerNavigationTests)
#include "ComputerNavigationTests.moc"
