/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EiHotKeyModifierMatchingTests.h"

#include "platform/EiComputer.h"

namespace {
constexpr KeyModifierMask AllLockModifiers = KeyModifierCapsLock | KeyModifierNumLock | KeyModifierScrollLock;
constexpr KeyModifierMask CommandModifiers[] = {
    KeyModifierShift, KeyModifierControl, KeyModifierAlt,        KeyModifierMeta,
    KeyModifierSuper, KeyModifierAltGr,   KeyModifierLevel5Lock,
};
constexpr KeyModifierMask LockModifiers[] = {
    0,
    KeyModifierCapsLock,
    KeyModifierNumLock,
    KeyModifierScrollLock,
    KeyModifierCapsLock | KeyModifierNumLock,
    KeyModifierCapsLock | KeyModifierScrollLock,
    KeyModifierNumLock | KeyModifierScrollLock,
    AllLockModifiers,
};
} // namespace

namespace deskflow {

void EiHotKeyModifierMatchingTests::findByMask_lockModifiers_matchHotkey_data()
{
  QTest::addColumn<KeyModifierMask>("modifiers");
  QTest::addColumn<KeyModifierMask>("lockModifiers");

  const KeyModifierMask modifiers[] = {
      0,
      KeyModifierShift,
      KeyModifierControl,
      KeyModifierAlt,
      KeyModifierMeta,
      KeyModifierSuper,
      KeyModifierAltGr,
      KeyModifierLevel5Lock,
      KeyModifierControl | KeyModifierShift,
  };
  for (auto mask : modifiers) {
    for (auto locks : LockModifiers) {
      const auto name = QByteArray::number(mask, 16) + '/' + QByteArray::number(locks, 16);
      QTest::newRow(name.constData()) << mask << locks;
    }
  }
}

void EiHotKeyModifierMatchingTests::findByMask_lockModifiers_matchHotkey()
{
  QFETCH(KeyModifierMask, modifiers);
  QFETCH(KeyModifierMask, lockModifiers);
  EiComputer::HotKeySet hotkeys(kKeyF12);
  hotkeys.addItem({modifiers, 1});

  const auto id = hotkeys.findByMask(modifiers | lockModifiers);

  QCOMPARE(id, 1u);
}

void EiHotKeyModifierMatchingTests::findByMask_differentCommandModifiers_doNotMatch_data()
{
  QTest::addColumn<KeyModifierMask>("registeredModifiers");
  QTest::addColumn<KeyModifierMask>("eventModifiers");

  for (auto mask : CommandModifiers) {
    const auto name = QByteArray::number(mask, 16);
    QTest::newRow((name + "/missing").constData()) << mask << AllLockModifiers;
    QTest::newRow((name + "/extra").constData()) << KeyModifierMask{0} << (mask | AllLockModifiers);
  }
  QTest::newRow("different") << KeyModifierControl << (KeyModifierShift | AllLockModifiers);
}

void EiHotKeyModifierMatchingTests::findByMask_differentCommandModifiers_doNotMatch()
{
  QFETCH(KeyModifierMask, registeredModifiers);
  QFETCH(KeyModifierMask, eventModifiers);
  EiComputer::HotKeySet hotkeys(kKeyF12);
  hotkeys.addItem({registeredModifiers, 1});

  const auto id = hotkeys.findByMask(eventModifiers);

  QCOMPARE(id, 0u);
}

void EiHotKeyModifierMatchingTests::findByMask_multipleHotkeys_selectsMatchingModifiers()
{
  EiComputer::HotKeySet hotkeys(kKeyF12);
  hotkeys.addItem({0, 1});
  hotkeys.addItem({KeyModifierControl, 2});
  hotkeys.addItem({KeyModifierControl | KeyModifierShift, 3});

  const auto id = hotkeys.findByMask(KeyModifierControl | KeyModifierShift | AllLockModifiers);

  QCOMPARE(id, 3u);
}

} // namespace deskflow

QTEST_GUILESS_MAIN(deskflow::EiHotKeyModifierMatchingTests)
