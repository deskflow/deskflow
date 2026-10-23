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
void loadUsKeymap(deskflow::EiKeyState &state)
{
  auto *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  xkb_rule_names names{};
  names.layout = "us";
  auto *map = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  char *text = xkb_keymap_get_as_string(map, XKB_KEYMAP_FORMAT_TEXT_V1);
  QTemporaryFile file;
  QVERIFY(file.open());
  file.write(text);
  file.flush();
  state.init(file.handle(), file.size());
  free(text);
  xkb_keymap_unref(map);
  xkb_context_unref(context);
}
deskflow::platform::WaylandKeyboardManager::Snapshot twoLayouts()
{
  deskflow::platform::WaylandKeyboardManager::Snapshot result;
  result.layouts = "us,ru";
  result.variants = ",";
  result.group = 1;
  result.configured = true;
  result.generation = 1;
  return result;
}
} // namespace

void EiKeyStateTests::captureLayout_preservesCompositorKeymap()
{
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadUsKeymap(state);
  auto *native = state.m_xkbKeymap;
  auto snapshot = twoLayouts();
  QCOMPARE(state.translateKey(38, snapshot), KeyID(0x0444));
  QCOMPARE(state.m_xkbKeymap, native);
  QCOMPARE(xkb_keymap_num_layouts(native), 1u);
  QCOMPARE(state.mapKeyFromKeyval(38), KeyID('a'));
  state.updateXkbState(50, true);
  QCOMPARE(state.translateKey(38, snapshot), KeyID(0x0424));
  snapshot.group = 0;
  QCOMPARE(state.translateKey(38, snapshot), KeyID('A'));
  state.updateXkbState(50, false);
  QCOMPARE(state.translateKey(38, snapshot), KeyID('a'));
}

void EiKeyStateTests::captureLayout_defersRebuildWhileShiftHeld()
{
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadUsKeymap(state);
  auto snapshot = twoLayouts();
  state.translateKey(38, snapshot);
  state.updateXkbState(50, true);
  snapshot.generation = 2;
  state.translateKey(38, snapshot);
  QCOMPARE(state.m_translationGeneration, uint64_t(1));
  state.updateXkbState(50, false);
  QCOMPARE(state.translateKey(38, snapshot), KeyID(0x0444));
  QCOMPARE(state.m_translationGeneration, uint64_t(2));
  QVERIFY(!(state.pollActiveModifiers() & KeyModifierShift));
}

void EiKeyStateTests::captureLayout_unknownGroupUsesCompositor()
{
  TestAppUtil util;
  EventQueue events;
  deskflow::EiKeyState state(nullptr, &events);
  loadUsKeymap(state);
  auto snapshot = twoLayouts();
  snapshot.group = 9;
  QCOMPARE(state.translateKey(38, snapshot), KeyID('a'));
  snapshot.group = -1;
  QCOMPARE(state.translateKey(38, snapshot), KeyID('a'));
}

QTEST_MAIN(EiKeyStateTests)
