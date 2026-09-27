/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "base/EventQueue.h"

#include <IOKit/hidsystem/IOHIDLib.h>

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

void OSXKeyStateTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void OSXKeyStateTests::mapModifiersFromOSX_OSXMask()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);

  KeyModifierMask outMask = 0;

  uint32_t shiftMask = 0 | kCGEventFlagMaskShift;
  outMask = keyState.mapModifiersFromOSX(shiftMask);
  QCOMPARE(outMask, KeyModifierShift);

  uint32_t ctrlMask = 0 | kCGEventFlagMaskControl;
  outMask = keyState.mapModifiersFromOSX(ctrlMask);
  QCOMPARE(outMask, KeyModifierControl);

  uint32_t altMask = 0 | kCGEventFlagMaskAlternate;
  outMask = keyState.mapModifiersFromOSX(altMask);
  QCOMPARE(outMask, KeyModifierAlt);

  uint32_t cmdMask = 0 | kCGEventFlagMaskCommand;
  outMask = keyState.mapModifiersFromOSX(cmdMask);
  QCOMPARE(outMask, KeyModifierSuper);

  uint32_t capsMask = 0 | kCGEventFlagMaskAlphaShift;
  outMask = keyState.mapModifiersFromOSX(capsMask);
  QCOMPARE(outMask, KeyModifierCapsLock);

  uint32_t numMask = 0 | kCGEventFlagMaskNumericPad;
  outMask = keyState.mapModifiersFromOSX(numMask);
  QCOMPARE(outMask, KeyModifierNumLock);
}

void OSXKeyStateTests::fakePollShift()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(SHIFT_ID_L, 0, 1, "en");
  QVERIFY(isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyDown(SHIFT_ID_R, 0, 2, "en");
  QVERIFY(isKeyPressed(keyState, kVK_RightShift + 1));

  keyState.fakeKeyUp(2);
  QVERIFY(!isKeyPressed(keyState, kVK_RightShift + 1));
}

void OSXKeyStateTests::modifierSides_data()
{
  QTest::addColumn<uint32_t>("left");
  QTest::addColumn<uint32_t>("right");
  QTest::addColumn<quint64>("mask");
  QTest::addColumn<quint64>("leftMask");
  QTest::addColumn<quint64>("rightMask");
  QTest::newRow("shift") << uint32_t(kVK_Shift) << uint32_t(kVK_RightShift) << quint64(kCGEventFlagMaskShift)
                         << quint64(NX_DEVICELSHIFTKEYMASK) << quint64(NX_DEVICERSHIFTKEYMASK);
  QTest::newRow("control") << uint32_t(kVK_Control) << uint32_t(kVK_RightControl) << quint64(kCGEventFlagMaskControl)
                           << quint64(NX_DEVICELCTLKEYMASK) << quint64(NX_DEVICERCTLKEYMASK);
  QTest::newRow("option") << uint32_t(kVK_Option) << uint32_t(kVK_RightOption) << quint64(kCGEventFlagMaskAlternate)
                          << quint64(NX_DEVICELALTKEYMASK) << quint64(NX_DEVICERALTKEYMASK);
  QTest::newRow("command") << uint32_t(kVK_Command) << uint32_t(kVK_RightCommand) << quint64(kCGEventFlagMaskCommand)
                           << quint64(NX_DEVICELCMDKEYMASK) << quint64(NX_DEVICERCMDKEYMASK);
}

void OSXKeyStateTests::modifierSides()
{
  QFETCH(uint32_t, left);
  QFETCH(uint32_t, right);
  QFETCH(quint64, mask);
  QFETCH(quint64, leftMask);
  QFETCH(quint64, rightMask);
  deskflow::KeyMap keyMap;
  EventQueue events;
  OSXKeyState state(&events, keyMap, {"en"}, false);
  const auto flags = [&] { return quint64(state.getKeyboardEventFlags()) & (mask | leftMask | rightMask); };
  for (bool caps : {false, true}) {
    state.setKeyboardModifiers(kVK_CapsLock, caps);
    for (bool rightFirst : {false, true}) {
      const auto first = rightFirst ? right : left;
      const auto second = rightFirst ? left : right;
      state.setKeyboardModifiers(first, true);
      QCOMPARE(flags(), mask | (rightFirst ? rightMask : leftMask));
      state.setKeyboardModifiers(second, true);
      QCOMPARE(flags(), mask | leftMask | rightMask);
      state.setKeyboardModifiers(first, false);
      QCOMPARE(flags(), mask | (rightFirst ? leftMask : rightMask));
      state.setKeyboardModifiers(second, false);
      QCOMPARE(flags(), quint64(0));
    }
  }
}

void OSXKeyStateTests::specialKeyMappings()
{
  deskflow::KeyMap keyMap;
  EventQueue events;
  OSXKeyState state(&events, keyMap, {"en"}, false);
  state.getKeyMapForSpecialKeys(keyMap, 0);
  keyMap.finish();
  for (const auto [id, vk] :
       {std::pair<KeyID, uint32_t>{kKeyHenkan, kVK_JIS_Kana},
        {kKeyZenkaku, kVK_JIS_Eisu},
        {kKeyShift_L, kVK_Shift},
        {kKeyShift_R, kVK_RightShift},
        {kKeyAlt_L, kVK_Option},
        {kKeyAlt_R, kVK_RightOption},
        {kKeyAltGr, kVK_RightOption}}) {
    const auto entries = keyMap.findCompatibleKey(id, 0, 0, 0);
    QVERIFY(entries != nullptr);
    QCOMPARE(entries->back().m_button, KeyButton(vk + 1));
    if (id == kKeyAltGr) {
      QCOMPARE(entries->back().m_generates, KeyModifierAlt);
    }
  }
}

void OSXKeyStateTests::fakePollChar()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(A_CHAR_ID, 0, 1, "en");
  QVERIFY(isKeyPressed(keyState, A_CHAR_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, A_CHAR_BUTTON));

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

void OSXKeyStateTests::fakePollCharWithModifier()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(A_CHAR_ID, KeyModifierShift, 1, "en");
  QVERIFY(isKeyPressed(keyState, A_CHAR_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, A_CHAR_BUTTON));

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

bool OSXKeyStateTests::isKeyPressed(const OSXKeyState &keyState, KeyButton button)
{
  // HACK: allow os to realize key state changes.
  Arch::sleep(.2);

  IKeyState::KeyButtonSet pressed;
  keyState.pollPressedKeys(pressed);

  IKeyState::KeyButtonSet::const_iterator it;
  for (it = pressed.begin(); it != pressed.end(); ++it) {
    LOG_DEBUG("checking key %d", *it);
    if (*it == button) {
      return true;
    }
  }
  return false;
}

QTEST_MAIN(OSXKeyStateTests)
