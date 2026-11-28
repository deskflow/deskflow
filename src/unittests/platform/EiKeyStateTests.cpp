/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EiKeyStateTests.h"

#include "base/EventQueue.h"
#include "deskflow/AppUtil.h"
#include "deskflow/KeyMap.h"
#include "deskflow/KeyTypes.h"
#include "platform/EiKeyState.h"

#include <QByteArray>
#include <QTemporaryFile>

#include <algorithm>
#include <cstdint>
#include <map>
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

// Key <AC01> as in the Polish layout: a, A, aogonek, Aogonek.
const char FourLevelAlphabeticKeymap[] = R"XKB(xkb_keymap {
xkb_keycodes "test" {
    minimum = 8;
    maximum = 255;
    <AC01> = 38;
    <LVL3> = 92;
};
xkb_types "test" {
    virtual_modifiers LevelThree;
    type "ONE_LEVEL" {
        modifiers = none;
        level_name[Level1] = "Any";
    };
    type "FOUR_LEVEL_ALPHABETIC" {
        modifiers = Shift + Lock + LevelThree;
        map[None] = Level1;
        map[Shift] = Level2;
        map[Lock] = Level2;
        map[LevelThree] = Level3;
        map[Shift+LevelThree] = Level4;
        map[Lock+LevelThree] = Level4;
        map[Lock+Shift+LevelThree] = Level3;
        level_name[Level1] = "Base";
        level_name[Level2] = "Shift";
        level_name[Level3] = "Alt Base";
        level_name[Level4] = "Shift Alt";
    };
};
xkb_compat "test" {
    virtual_modifiers LevelThree;
    interpret ISO_Level3_Shift+AnyOfOrNone(all) {
        virtualModifier = LevelThree;
        action = SetMods(modifiers=LevelThree);
    };
};
xkb_symbols "test" {
    key <AC01> { type = "FOUR_LEVEL_ALPHABETIC", [ a, A, aogonek, Aogonek ] };
    key <LVL3> { [ ISO_Level3_Shift ] };
    modifier_map Mod5 { <LVL3> };
};
};)XKB";

class TestEiKeyState : public deskflow::EiKeyState
{
public:
  using EiKeyState::EiKeyState;
  using EiKeyState::getKeyMap;
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

void EiKeyStateTests::getKeyMap_fourLevelAlphabetic_altGrLevelDoesNotRequireCapsLock()
{
  TestAppUtil appUtil;
  EventQueue eventQueue;
  TestEiKeyState keyState(nullptr, &eventQueue);

  QTemporaryFile keymapFile;
  QVERIFY(keymapFile.open());
  const QByteArray keymapData =
      QByteArray::fromRawData(FourLevelAlphabeticKeymap, sizeof(FourLevelAlphabeticKeymap) - 1);
  QCOMPARE(keymapFile.write(keymapData), keymapData.size());
  QVERIFY(keymapFile.flush());
  keyState.init(keymapFile.handle(), keymapFile.size());

  deskflow::KeyMap keyMap;
  keyState.getKeyMap(keyMap);

  std::map<KeyID, std::vector<KeyModifierMask>> required;
  keyMap.foreachKey(
      [](KeyID id, int32_t, deskflow::KeyMap::KeyItem &item, void *userData) {
        (*static_cast<std::map<KeyID, std::vector<KeyModifierMask>> *>(userData))[id].push_back(item.m_required);
      },
      &required
  );

  constexpr KeyID aogonek = 0x0105;
  constexpr KeyID Aogonek = 0x0104;

  // Level 3 is reached with AltGr alone; Caps Lock or Shift would give Level 4
  QCOMPARE(required[aogonek], std::vector<KeyModifierMask>{KeyModifierAltGr});

  // Level 4 is reachable with either Shift or Caps Lock
  QCOMPARE(required[Aogonek].size(), 2u);
  QVERIFY(std::ranges::count(required[Aogonek], KeyModifierAltGr | KeyModifierShift) == 1);
  QVERIFY(std::ranges::count(required[Aogonek], KeyModifierAltGr | KeyModifierCapsLock) == 1);
}

void EiKeyStateTests::getKeyMap_fourLevelAlphabetic_baseLevelSensitiveToAltGr()
{
  TestAppUtil appUtil;
  EventQueue eventQueue;
  TestEiKeyState keyState(nullptr, &eventQueue);

  QTemporaryFile keymapFile;
  QVERIFY(keymapFile.open());
  const QByteArray keymapData =
      QByteArray::fromRawData(FourLevelAlphabeticKeymap, sizeof(FourLevelAlphabeticKeymap) - 1);
  QCOMPARE(keymapFile.write(keymapData), keymapData.size());
  QVERIFY(keymapFile.flush());
  keyState.init(keymapFile.handle(), keymapFile.size());

  deskflow::KeyMap keyMap;
  keyState.getKeyMap(keyMap);

  std::map<KeyID, std::vector<deskflow::KeyMap::KeyItem>> items;
  keyMap.foreachKey(
      [](KeyID id, int32_t, deskflow::KeyMap::KeyItem &item, void *userData) {
        (*static_cast<std::map<KeyID, std::vector<deskflow::KeyMap::KeyItem>> *>(userData))[id].push_back(item);
      },
      &items
  );

  constexpr KeyID a = 0x0061;

  // Level 1 needs no modifiers, but AltGr changes the level, so it must be released
  // when typing it (e.g. the base key after a dead key typed with AltGr)
  QCOMPARE(items[a].size(), 1u);
  QCOMPARE(items[a].front().m_required, KeyModifierMask{0});
  QVERIFY((items[a].front().m_sensitive & KeyModifierAltGr) != 0);
}

QTEST_MAIN(EiKeyStateTests)
