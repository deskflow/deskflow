/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "../deskflow/MockEventQueue.h"
#include "base/EventQueue.h"

#include <vector>

namespace {
// records every key event the key state sends
class KeyEventRecorder : public MockEventQueue
{
public:
  void addEvent(Event &&event) override
  {
    const auto *info = static_cast<const IKeyState::KeyInfo *>(event.getData());
    m_downs.push_back(event.getType() == EventTypes::KeyStateKeyDown);
    m_keyIds.push_back(info->m_key);
    m_buttons.push_back(info->m_button);
    Event::deleteData(event);
  }

  std::vector<bool> m_downs;
  std::vector<KeyID> m_keyIds;
  std::vector<KeyButton> m_buttons;
};
} // namespace

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
  QVERIFY(isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyUp(2);
  QVERIFY(!isKeyPressed(keyState, SHIFT_BUTTON));
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

void OSXKeyStateTests::handleModifierKeys_sidedVirtualKey_sendsSameSide_data()
{
  QTest::addColumn<quint32>("mask");
  QTest::addColumn<quint32>("virtualKey");
  QTest::addColumn<quint32>("keyId");

  QTest::newRow("shift left") << quint32(KeyModifierShift) << quint32(kVK_Shift) << quint32(kKeyShift_L);
  QTest::newRow("shift right") << quint32(KeyModifierShift) << quint32(kVK_RightShift) << quint32(kKeyShift_R);
  QTest::newRow("control left") << quint32(KeyModifierControl) << quint32(kVK_Control) << quint32(kKeyControl_L);
  QTest::newRow("control right") << quint32(KeyModifierControl) << quint32(kVK_RightControl) << quint32(kKeyControl_R);
  QTest::newRow("option left") << quint32(KeyModifierAlt) << quint32(kVK_Option) << quint32(kKeyAlt_L);
  QTest::newRow("option right") << quint32(KeyModifierAlt) << quint32(kVK_RightOption) << quint32(kKeyAlt_R);
  QTest::newRow("command left") << quint32(KeyModifierSuper) << quint32(kVK_Command) << quint32(kKeySuper_L);
  QTest::newRow("command right") << quint32(KeyModifierSuper) << quint32(kVK_RightCommand) << quint32(kKeySuper_R);
}

void OSXKeyStateTests::handleModifierKeys_sidedVirtualKey_sendsSameSide()
{
  QFETCH(quint32, mask);
  QFETCH(quint32, virtualKey);
  QFETCH(quint32, keyId);
  deskflow::KeyMap keyMap;
  KeyEventRecorder events;
  OSXKeyState keyState(&events, keyMap, {"en"}, true);

  keyState.handleModifierKeys(nullptr, 0, mask, virtualKey);
  keyState.handleModifierKeys(nullptr, mask, 0, virtualKey);

  const KeyButton button = virtualKey + 1;
  QCOMPARE(events.m_downs, std::vector<bool>({true, false}));
  QCOMPARE(events.m_keyIds, std::vector<KeyID>({keyId, keyId}));
  QCOMPARE(events.m_buttons, std::vector<KeyButton>({button, button}));
}

void OSXKeyStateTests::handleModifierKeys_bothSidesHeld_releasesPressedSide_data()
{
  QTest::addColumn<quint32>("firstDown");
  QTest::addColumn<quint32>("secondDown");
  QTest::addColumn<quint32>("firstUp");
  QTest::addColumn<quint32>("secondUp");
  QTest::addColumn<quint32>("keyId");

  QTest::newRow("right first, right released first")
      << quint32(kVK_RightCommand) << quint32(kVK_Command) << quint32(kVK_RightCommand) << quint32(kVK_Command)
      << quint32(kKeySuper_R);
  QTest::newRow("right first, left released first")
      << quint32(kVK_RightCommand) << quint32(kVK_Command) << quint32(kVK_Command) << quint32(kVK_RightCommand)
      << quint32(kKeySuper_R);
  QTest::newRow("left first, left released first")
      << quint32(kVK_Command) << quint32(kVK_RightCommand) << quint32(kVK_Command) << quint32(kVK_RightCommand)
      << quint32(kKeySuper_L);
  QTest::newRow("left first, right released first")
      << quint32(kVK_Command) << quint32(kVK_RightCommand) << quint32(kVK_RightCommand) << quint32(kVK_Command)
      << quint32(kKeySuper_L);
}

void OSXKeyStateTests::handleModifierKeys_bothSidesHeld_releasesPressedSide()
{
  QFETCH(quint32, firstDown);
  QFETCH(quint32, secondDown);
  QFETCH(quint32, firstUp);
  QFETCH(quint32, secondUp);
  QFETCH(quint32, keyId);
  deskflow::KeyMap keyMap;
  KeyEventRecorder events;
  OSXKeyState keyState(&events, keyMap, {"en"}, true);

  // the mask only changes on the first press and the last release, so only
  // those send a key event, and the release is sent for the pressed side.
  keyState.handleModifierKeys(nullptr, 0, KeyModifierSuper, firstDown);
  keyState.handleModifierKeys(nullptr, KeyModifierSuper, KeyModifierSuper, secondDown);
  keyState.handleModifierKeys(nullptr, KeyModifierSuper, KeyModifierSuper, firstUp);
  keyState.handleModifierKeys(nullptr, KeyModifierSuper, 0, secondUp);

  const KeyButton button = firstDown + 1;
  QCOMPARE(events.m_downs, std::vector<bool>({true, false}));
  QCOMPARE(events.m_keyIds, std::vector<KeyID>({keyId, keyId}));
  QCOMPARE(events.m_buttons, std::vector<KeyButton>({button, button}));
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
