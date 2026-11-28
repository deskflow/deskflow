/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EiKeyStateTests.h"

#include "base/EventQueue.h"
#include "deskflow/AppUtil.h"
#include "deskflow/KeyTypes.h"
#include "platform/EiKeyState.h"

#include <QByteArray>
#include <QTemporaryFile>

#include <cstdint>
#include <utility>
#include <vector>

namespace {
class TestAppUtil : public AppUtil
{
public:
  int run() override
  {
    return 0;
  }

  std::vector<std::string> getKeyboardLayoutList() override
  {
    return {"en"};
  }

  std::string getCurrentLanguageCode() override
  {
    return "en";
  }
};

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

// A keymap where the Menu key (xkb keycode 135, evdev 127) is remapped to
// the compose key, as the xkb option "compose:menu" would do.
const char ComposeKeymap[] = R"XKB(xkb_keymap {
xkb_keycodes "test" {
    minimum = 8;
    maximum = 255;
    <MENU> = 135;
    <AB01> = 38;
};
xkb_types "test" {
    type "ONE_LEVEL" {
        modifiers = none;
        level_name[Level1] = "Any";
    };
};
xkb_compat "test" {
};
xkb_symbols "test" {
    key <MENU> { [ Multi_key ] };
    key <AB01> { [ q ] };
};
};)XKB";

constexpr std::uint32_t MenuKeycode = 135;

// An EiKeyState that records the buttons injected via fakeKey() instead of
// forwarding them to a computer.
class RecordingEiKeyState : public deskflow::EiKeyState
{
public:
  using deskflow::EiKeyState::EiKeyState;

  std::vector<std::pair<KeyButton, bool>> fakeKeys;

protected:
  void fakeKey(const deskflow::KeyMap::Keystroke &keystroke) override
  {
    fakeKeys.emplace_back(keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_press);
  }
};
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

void EiKeyStateTests::getKeyMap_composeKeyOnMenu_aliasesMenuKey()
{
  TestAppUtil appUtil;
  EventQueue eventQueue;
  RecordingEiKeyState keyState(nullptr, &eventQueue);

  QTemporaryFile keymapFile;
  QVERIFY(keymapFile.open());
  const QByteArray keymapData = QByteArray::fromRawData(ComposeKeymap, sizeof(ComposeKeymap) - 1);
  QCOMPARE(keymapFile.write(keymapData), keymapData.size());
  QVERIFY(keymapFile.flush());
  keyState.init(keymapFile.handle(), keymapFile.size());

  keyState.updateKeyMap();

  // The server (e.g. Windows) sends Menu; the alias entry must map it to the
  // same button as the compose key.
  keyState.fakeKeyDown(kKeyMenu, 0, 0, "en");

  QVERIFY2(keyState.fakeKeys.size() == 1, "expected exactly one injected key press");
  QCOMPARE(keyState.fakeKeys[0].first, static_cast<KeyButton>(MenuKeycode - 8));
  QCOMPARE(keyState.fakeKeys[0].second, true);
}

QTEST_MAIN(EiKeyStateTests)
