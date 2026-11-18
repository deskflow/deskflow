/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EiKeyStateTests.h"

#include "EiTestFixtures.h"
#include "base/EventQueue.h"
#include "deskflow/KeyTypes.h"
#include "platform/EiKeyState.h"

#include <QByteArray>
#include <QTemporaryFile>

#include <cstdint>

using ei_test::CaptureKeymap;
using ei_test::TestAppUtil;

namespace {
const char TestKeymap[] = R"XKB(xkb_keymap {
xkb_keycodes "test" {
    minimum = 8;
    maximum = 255;
    <LFSH> = 50;
    <NMLK> = 77;
};
xkb_types "test" {
    type "ONE_LEVEL" {
        modifiers = none;
        level_name[Level1] = "Any";
    };
};
xkb_compat "test" {
    interpret Shift_L+AnyOfOrNone(all) {
        action = SetMods(modifiers=Shift);
    };
    interpret Num_Lock+AnyOfOrNone(all) {
        action = LockMods(modifiers=Mod2);
    };
};
xkb_symbols "test" {
    key <LFSH> { [ Shift_L ] };
    key <NMLK> { [ Num_Lock ] };
    modifier_map Shift { <LFSH> };
    modifier_map Mod2 { <NMLK> };
};
};)XKB";

// XKB keycodes for TestKeymap.
constexpr std::uint32_t LeftShiftKeycode = 50;
constexpr std::uint32_t NumLockKeycode = 77;
} // namespace

void EiKeyStateTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void EiKeyStateTests::clearStaleModifiers_shiftDownAndNumLockOn_shiftClearedAndNumLockPreserved()
{
  TestAppUtil appUtil;
  EventQueue eventQueue;
  deskflow::EiKeyState keyState(nullptr, &eventQueue);

  QTemporaryFile keymapFile;
  QVERIFY(keymapFile.open());
  const QByteArray keymapData = QByteArray::fromRawData(TestKeymap, sizeof(TestKeymap) - 1);
  QCOMPARE(keymapFile.write(keymapData), keymapData.size());
  QVERIFY(keymapFile.flush());
  keyState.init(keymapFile.handle(), keymapFile.size());

  keyState.updateXkbState(LeftShiftKeycode, true);
  keyState.updateXkbState(NumLockKeycode, true);
  keyState.updateXkbState(NumLockKeycode, false);

  QVERIFY((keyState.pollActiveModifiers() & KeyModifierShift) != 0);
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierNumLock) != 0);

  keyState.clearStaleModifiers();

  QVERIFY((keyState.pollActiveModifiers() & KeyModifierShift) == 0);
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierNumLock) != 0);
}

void EiKeyStateTests::updateLockedModifiers_compositorLockState_numLockFollowsCompositor()
{
  // Real modifier Mod2 is index 4 (Shift, Lock, Control, Mod1, Mod2, ...).
  constexpr std::uint32_t xkbMod2Mask = 1 << 4;

  TestAppUtil appUtil;
  EventQueue eventQueue;
  deskflow::EiKeyState keyState(nullptr, &eventQueue);

  QTemporaryFile keymapFile;
  QVERIFY(keymapFile.open());
  const QByteArray keymapData = QByteArray::fromRawData(TestKeymap, sizeof(TestKeymap) - 1);
  QCOMPARE(keymapFile.write(keymapData), keymapData.size());
  QVERIFY(keymapFile.flush());
  keyState.init(keymapFile.handle(), keymapFile.size());

  keyState.updateXkbState(LeftShiftKeycode, true);
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierNumLock) == 0);

  // NumLock toggled on locally, reported by the compositor
  keyState.updateLockedModifiers(xkbMod2Mask);
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierNumLock) != 0);
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierShift) != 0);

  keyState.clearStaleModifiers();
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierNumLock) != 0);

  // NumLock toggled off locally
  keyState.updateLockedModifiers(0);
  QVERIFY((keyState.pollActiveModifiers() & KeyModifierNumLock) == 0);
}

namespace {
void loadCaptureKeymap(deskflow::EiKeyState &state)
{
  QTemporaryFile file;
  QVERIFY(file.open());
  QCOMPARE(file.write(CaptureKeymap), qint64(sizeof(CaptureKeymap) - 1));
  QVERIFY(file.flush());
  state.init(file.handle(), file.size());
}
} // namespace

void EiKeyStateTests::captureModifiers_data()
{
  QTest::addColumn<quint32>("depressed");
  QTest::addColumn<quint32>("latched");
  QTest::addColumn<quint32>("locked");
  QTest::addColumn<quint32>("group");
  QTest::addColumn<quint32>("keycode");
  QTest::addColumn<KeyID>("expected");
  QTest::addColumn<KeyModifierMask>("modifiers");
  QTest::newRow("English") << 0u << 0u << 0u << 0u << 24u << KeyID('q') << KeyModifierMask{0};
  QTest::newRow("Russian-first-key") << 0u << 0u << 0u << 1u << 24u << KeyID(0x0439) << KeyModifierMask{0};
  QTest::newRow("Shift") << 1u << 0u << 0u << 1u << 24u << KeyID(0x0419) << KeyModifierShift;
  QTest::newRow("Caps") << 0u << 0u << 2u << 1u << 24u << KeyID(0x0419) << KeyModifierCapsLock;
  QTest::newRow("Shift-Caps") << 1u << 0u << 2u << 0u << 24u << KeyID('q')
                              << KeyModifierMask(KeyModifierShift | KeyModifierCapsLock);
  QTest::newRow("latched-Shift") << 0u << 1u << 0u << 0u << 24u << KeyID('Q') << KeyModifierShift;
  QTest::newRow("AltGr-single-group-key") << 128u << 0u << 0u << 1u << 26u << KeyID(0x20ac) << KeyModifierAltGr;
  QTest::newRow("NumLock") << 0u << 0u << 16u << 0u << 91u << KeyID(kKeyKP_1) << KeyModifierNumLock;
}

void EiKeyStateTests::captureModifiers()
{
  QFETCH(quint32, depressed);
  QFETCH(quint32, latched);
  QFETCH(quint32, locked);
  QFETCH(quint32, group);
  QFETCH(quint32, keycode);
  QFETCH(KeyID, expected);
  QFETCH(KeyModifierMask, modifiers);
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadCaptureKeymap(state);
  for (int i = 0; i < 3; ++i) {
    state.updateCaptureModifiers(depressed, latched, locked, group);
    QCOMPARE(state.mapKeyFromKeyval(keycode), expected);
    QCOMPARE(state.pollActiveGroup(), group);
    QCOMPARE(state.pollActiveModifiers(), modifiers);
    // Captured key actions must never mutate the authoritative snapshot,
    // including consuming a latch or toggling a lock/group twice.
    state.updateXkbState(keycode, true);
    state.updateXkbState(keycode, false);
    state.updateXkbState(90, true);
    state.updateXkbState(90, false);
    state.updateXkbState(92, true);
    state.updateXkbState(92, false);
    QCOMPARE(state.mapKeyFromKeyval(keycode), expected);
    QCOMPARE(state.pollActiveGroup(), group);
  }
}

void EiKeyStateTests::fallbackAndDelayedSnapshot()
{
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadCaptureKeymap(state);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('q'));
  state.updateXkbState(50, true);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('Q'));
  state.updateXkbState(92, true);
  state.updateXkbState(92, false);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID(0x0419));
  state.updateCaptureModifiers(0, 0, 0, 0);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('q'));
  // Invalid snapshots select the independently maintained fallback state.
  state.updateCaptureModifiers(0, 0, 0, 2);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID(0x0419));
  state.updateCaptureModifiers(0, 0, 0, XKB_LAYOUT_INVALID);
  QCOMPARE(state.pollActiveGroup(), 1);
  state.updateCaptureModifiers(0, 0, 0, 1);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID(0x0439));
}

void EiKeyStateTests::functionAndUnmappedKeys()
{
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadCaptureKeymap(state);
  for (bool capture : {false, true}) {
    for (quint32 group : {0u, 1u}) {
      for (quint32 modifiers : {0u, 1u, 4u, 8u, 4u | 8u, 1u | 4u | 8u}) {
        if (capture)
          state.updateCaptureModifiers(modifiers, 0, 0, group);
        else
          xkb_state_update_mask(state.m_xkbState, modifiers, 0, 0, 0, 0, group);
        const bool switchVT = (modifiers & (4u | 8u)) == (4u | 8u);
        for (quint32 i = 0; i < 12; ++i)
          QCOMPARE(state.mapKeyFromKeyval(67 + i), KeyID(switchVT ? kKeyNone : kKeyF1 + i));
        for (quint32 code : {0u, 8u, 93u, 94u, 255u, 999u})
          QCOMPARE(state.mapKeyFromKeyval(code), KeyID(kKeyNone));
      }
    }
  }
}

void EiKeyStateTests::captureResetAndKeymapReplacement()
{
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadCaptureKeymap(state);
  state.updateCaptureModifiers(1, 0, 16, 1);
  state.updateXkbState(50, true);
  state.updateKeyState();
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID(0x0419));
  state.clearStaleModifiers();
  QCOMPARE(state.pollActiveGroup(), 1);
  QVERIFY(state.pollActiveModifiers() & KeyModifierNumLock);
  state.resetCaptureState();
  QCOMPARE(state.pollActiveModifiers(), KeyModifierMask{0});
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('q'));
  state.updateCaptureModifiers(0, 0, 0, 1);
  loadCaptureKeymap(state);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('q'));
  state.updateCaptureModifiers(0, 0, 0, 1);
  QTemporaryFile replacement;
  QVERIFY(replacement.open());
  QCOMPARE(replacement.write(TestKeymap), qint64(sizeof(TestKeymap) - 1));
  QVERIFY(replacement.flush());
  state.init(replacement.handle(), replacement.size());
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID(kKeyNone));
  QCOMPARE(state.pollActiveModifiers(), KeyModifierMask{0});
  QCOMPARE(state.pollActiveGroup(), 0);
  state.updateCaptureModifiers(0, 0, 0, 1);
  QVERIFY(!state.m_haveCaptureModifiers);
  state.initDefaultKeymap();
  QVERIFY(!state.m_haveCaptureModifiers);
}

QTEST_MAIN(EiKeyStateTests)
