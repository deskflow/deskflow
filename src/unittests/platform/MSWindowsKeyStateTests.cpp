/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsKeyStateTests.h"

#include "base/EventQueue.h"
#include "platform/MSWindowsKeyEvent.h"
#include "platform/MSWindowsKeyState.h"

void MSWindowsKeyStateTests::initTestCase()
{
  m_arch.init();
}

void MSWindowsKeyStateTests::remappedKeys_data()
{
  QTest::addColumn<uint32_t>("vk");
  QTest::addColumn<uint32_t>("button");
  QTest::addColumn<uint32_t>("expected");
  QTest::newRow("ime-on-no-scan") << uint32_t(VK_IME_ON) << 0u << kKeyHenkan;
  QTest::newRow("ime-off-no-scan") << uint32_t(VK_IME_OFF) << 0u << kKeyZenkaku;
  QTest::newRow("ime-on-extended") << uint32_t(VK_IME_ON) << 0x170u << kKeyHenkan;
  QTest::newRow("ime-off-extended") << uint32_t(VK_IME_OFF) << 0x170u << kKeyZenkaku;
  QTest::newRow("left-shift") << uint32_t(VK_LSHIFT) << 0x2au << kKeyShift_L;
  QTest::newRow("right-shift") << uint32_t(VK_RSHIFT) << 0x36u << kKeyShift_R;
  QTest::newRow("generic-right-shift") << uint32_t(VK_SHIFT) << 0x36u << kKeyShift_R;
  QTest::newRow("generic-left-shift") << uint32_t(VK_SHIFT) << 0x2au << kKeyShift_L;
  QTest::newRow("left-control") << uint32_t(VK_LCONTROL) << 0x1du << kKeyControl_L;
  QTest::newRow("right-control") << uint32_t(VK_RCONTROL) << 0x11du << kKeyControl_R;
  QTest::newRow("left-alt") << uint32_t(VK_LMENU) << 0x38u << kKeyAlt_L;
  QTest::newRow("right-alt") << uint32_t(VK_RMENU) << 0x138u << kKeyAltGr;
}

void MSWindowsKeyStateTests::remappedKeys()
{
  QFETCH(uint32_t, vk);
  QFETCH(uint32_t, button);
  QFETCH(uint32_t, expected);
  deskflow::KeyMap keyMap;
  EventQueue events;
  MSWindowsKeyState state(nullptr, nullptr, &events, keyMap, {"en"}, false);
  const WPARAM message = static_cast<WPARAM>(vk) << 16;
  const LPARAM info = static_cast<LPARAM>(button) << 16;
  QCOMPARE(state.mapKeyFromEvent(message, info, nullptr), expected);
  QCOMPARE(state.mapKeyFromEvent(message, info | 0x80000000u, nullptr), expected);
}

void MSWindowsKeyStateTests::virtualKeyOnlyInputHasStableButtons()
{
  using namespace deskflow::windows;
  deskflow::KeyMap keyMap;
  EventQueue events;
  MSWindowsKeyState state(nullptr, nullptr, &events, keyMap, {"en"}, false);
  const auto on = state.virtualKeyToButton(VK_IME_ON);
  const auto off = state.virtualKeyToButton(VK_IME_OFF);
  QVERIFY(on != off);
  QVERIFY(on > 0 && on < IKeyState::s_numButtons);
  QVERIFY(off > 0 && off < IKeyState::s_numButtons);
  QCOMPARE(state.mapButtonToVirtualKey(on), UINT(VK_IME_ON));
  QCOMPARE(state.mapButtonToVirtualKey(off), UINT(VK_IME_OFF));

  // The same scan code (or no scan code) from Windows must not alias IME
  // commands or modifiers. Preserve repeat/up/context bits during repair.
  for (const auto vk : {VK_IME_ON, VK_IME_OFF}) {
    for (const LPARAM info : {LPARAM(1), LPARAM(0x00380001), LPARAM(0xe1380001)}) {
      const auto normalized = normalizeKeyEvent(vk, info, 0);
      QCOMPARE((normalized >> 16) & 0x1ff, LPARAM(imeButton(vk)));
      QCOMPARE(normalized & ~LPARAM(0x01ff0000), info & ~LPARAM(0x01ff0000));
      QCOMPARE(state.mapKeyFromEvent(WPARAM(vk) << 16, normalized, nullptr), imeKeyID(vk));
    }
  }
  QCOMPARE(normalizeKeyEvent(VK_RSHIFT, 1, 0x36), LPARAM(0x00360001));
  QCOMPARE(normalizeKeyEvent(VK_RMENU, 1, 0x138), LPARAM(0x01380001));
  QCOMPARE(normalizeKeyEvent(VK_RSHIFT, 0x00360001, 0), LPARAM(0x00360001));

  // An intervening IME command must not release or repeat a held modifier.
  state.onKey(0x36, true, KeyModifierShift);
  QVERIFY(!state.testAutoRepeat(true, false, on));
  state.onKey(on, true, KeyModifierShift);
  QVERIFY(!state.testAutoRepeat(true, false, off));
  state.onKey(off, true, KeyModifierShift);
  state.onKey(on, false, KeyModifierShift);
  QVERIFY(!state.isKeyDown(on));
  QVERIFY(state.isKeyDown(off));
  QVERIFY(state.isKeyDown(0x36));
  state.onKey(off, false, KeyModifierShift);
  state.onKey(0x36, false, 0);
}

QTEST_MAIN(MSWindowsKeyStateTests)
