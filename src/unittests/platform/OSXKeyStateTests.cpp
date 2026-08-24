/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "base/EventQueue.h"
#include "platform/IOSXKeyResource.h"

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

namespace {
class RecordingEventQueue : public EventQueue
{
public:
  void addEvent(Event &&event) override
  {
    m_events.push_back(std::move(event));
  }

  const std::vector<Event> &events() const
  {
    return m_events;
  }

private:
  std::vector<Event> m_events;
};

class DeadKeyResource : public IOSXKeyResource
{
public:
  explicit DeadKeyResource(KeyID deadKeyOutput, bool hasDirectOutput = false)
      : m_deadKeyOutput(deadKeyOutput),
        m_hasDirectOutput(hasDirectOutput)
  {
  }

  bool isValid() const override
  {
    return true;
  }

  uint32_t getNumModifierCombinations() const override
  {
    return 1;
  }

  uint32_t getNumTables() const override
  {
    return 1;
  }

  uint32_t getNumButtons() const override
  {
    return 3;
  }

  uint32_t getTableForModifier(uint32_t) const override
  {
    return 0;
  }

  KeyID getKey(uint32_t, uint32_t button) const override
  {
    if (button == 0) {
      return kKeyDeadAcute;
    }
    if (button == 1) {
      return ' ';
    }
    return m_hasDirectOutput ? m_deadKeyOutput : kKeyNone;
  }

  KeyID getDeadKeyOutput(uint32_t, uint32_t button) const override
  {
    return button == 0 ? m_deadKeyOutput : kKeyNone;
  }

private:
  KeyID m_deadKeyOutput;
  bool m_hasDirectOutput;
};

class AmbiguousDeadKeyResource : public IOSXKeyResource
{
public:
  bool isValid() const override
  {
    return true;
  }

  uint32_t getNumModifierCombinations() const override
  {
    return 1;
  }

  uint32_t getNumTables() const override
  {
    return 1;
  }

  uint32_t getNumButtons() const override
  {
    return 3;
  }

  uint32_t getTableForModifier(uint32_t) const override
  {
    return 0;
  }

  KeyID getKey(uint32_t, uint32_t button) const override
  {
    if (button < 2) {
      return kKeyDeadAcute;
    }
    return button == 2 ? static_cast<KeyID>(' ') : kKeyNone;
  }

  KeyID getDeadKeyOutput(uint32_t, uint32_t button) const override
  {
    if (button == 0) {
      return 0x00b4;
    }
    return button == 1 ? static_cast<KeyID>('\'') : kKeyNone;
  }
};

class ModifierSensitiveDeadKeyResource : public IOSXKeyResource
{
public:
  bool isValid() const override
  {
    return true;
  }

  uint32_t getNumModifierCombinations() const override
  {
    return 4;
  }

  uint32_t getNumTables() const override
  {
    return 2;
  }

  uint32_t getNumButtons() const override
  {
    return 2;
  }

  uint32_t getTableForModifier(uint32_t mask) const override
  {
    return (mask & 2) != 0 ? 1 : 0;
  }

  KeyID getKey(uint32_t, uint32_t button) const override
  {
    return button == 0 ? kKeyDeadAcute : static_cast<KeyID>(' ');
  }

  KeyID getDeadKeyOutput(uint32_t table, uint32_t button) const override
  {
    if (button != 0) {
      return kKeyNone;
    }
    return table == 0 ? static_cast<KeyID>('\'') : static_cast<KeyID>(0x00b4);
  }
};

} // namespace

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

void OSXKeyStateTests::getKeyMap_addsLayoutSpecificDeadKeyOutput()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"pt"}, true);
  DeadKeyResource resource('\'');

  QVERIFY(keyState.getKeyMap(keyMap, 0, resource));
  keyMap.finish();

  const auto *entry = keyMap.findCompatibleKey('\'', 0, 0, 0);
  QVERIFY(entry != nullptr);
  QCOMPARE(entry->size(), 2);
  QCOMPARE(entry->at(0).m_id, kKeyDeadAcute);
  QCOMPARE(entry->at(1).m_id, static_cast<KeyID>(' '));
}

void OSXKeyStateTests::getKeyMap_usesDeadKeyThatProducesSpacingOutput()
{
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, {"pt"}, true);
  deskflow::KeyMap keyMap;
  AmbiguousDeadKeyResource resource;

  keyState.getKeyMap(keyMap, 0, resource);
  keyMap.finish();

  const auto *apostrophe = keyMap.findCompatibleKey('\'', 0, 0, 0);
  QVERIFY(apostrophe != nullptr);
  QCOMPARE(apostrophe->size(), 2);
  QCOMPARE(apostrophe->at(0).m_button, 2);

  const auto *acute = keyMap.findCompatibleKey(0x00b4, 0, 0, 0);
  QVERIFY(acute != nullptr);
  QCOMPARE(acute->size(), 2);
  QCOMPARE(acute->at(0).m_button, 1);
}

void OSXKeyStateTests::getKeyMap_distinguishesDeadKeyOutputsByModifier()
{
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, {"pt"}, true);
  deskflow::KeyMap keyMap;
  ModifierSensitiveDeadKeyResource resource;

  QVERIFY(keyState.getKeyMap(keyMap, 0, resource));
  keyMap.finish();

  const auto *apostrophe = keyMap.findCompatibleKey('\'', 0, 0, KeyModifierShift);
  QVERIFY(apostrophe != nullptr);
  QCOMPARE(apostrophe->at(0).m_required, 0);

  const auto *acute = keyMap.findCompatibleKey(0x00b4, 0, KeyModifierShift, KeyModifierShift);
  QVERIFY(acute != nullptr);
  QCOMPARE(acute->at(0).m_required, KeyModifierShift);
}

void OSXKeyStateTests::getKeyMap_doesNotAssumeDeadKeyOutput()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"pt"}, true);
  DeadKeyResource resource(0x00b4);

  QVERIFY(keyState.getKeyMap(keyMap, 0, resource));
  keyMap.finish();

  const auto *entry = keyMap.findCompatibleKey(0x00b4, 0, 0, 0);
  QVERIFY(entry != nullptr);
  QCOMPARE(entry->size(), 2);
  QCOMPARE(entry->at(0).m_id, kKeyDeadAcute);
  QCOMPARE(entry->at(1).m_id, static_cast<KeyID>(' '));
  QVERIFY(keyMap.findCompatibleKey('\'', 0, 0, 0) == nullptr);
}

void OSXKeyStateTests::getKeyMap_preservesDirectDeadKeyOutput()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"pt"}, true);
  DeadKeyResource resource('\'', true);

  QVERIFY(keyState.getKeyMap(keyMap, 0, resource));
  keyMap.finish();

  const auto *entry = keyMap.findCompatibleKey('\'', 0, 0, 0);
  QVERIFY(entry != nullptr);
  QCOMPARE(entry->size(), 1);
  QCOMPARE(entry->at(0).m_id, static_cast<KeyID>('\''));
}

void OSXKeyStateTests::sendKeyEvents_multiCharacterButton_releasesIntermediateKey()
{
  deskflow::KeyMap keyMap;
  RecordingEventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"pt"}, true);

  keyState.sendKeyEvents(this, true, false, {'\'', 'm'}, 0, 47);
  keyState.sendKeyEvents(this, false, false, {kKeyNone}, 0, 47);
  keyState.sendKeyEvents(this, true, false, {'\''}, 0, 48);
  keyState.sendKeyEvents(this, true, true, {'m'}, 0, 48);
  keyState.sendKeyEvents(this, false, false, {kKeyNone}, 0, 48);

  const std::vector<EventTypes> expected = {EventTypes::KeyStateKeyDown, EventTypes::KeyStateKeyUp,
                                            EventTypes::KeyStateKeyDown, EventTypes::KeyStateKeyUp,
                                            EventTypes::KeyStateKeyDown, EventTypes::KeyStateKeyRepeat,
                                            EventTypes::KeyStateKeyUp};
  const std::vector<KeyID> expectedKeys = {'\'', kKeyNone, 'm', kKeyNone, '\'', 'm', kKeyNone};
  const std::vector<KeyButton> expectedButtons = {47, 47, 47, 47, 48, 48, 48};
  QCOMPARE(eventQueue.events().size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    QCOMPARE(eventQueue.events().at(i).getType(), expected.at(i));
    const auto *info = static_cast<IKeyState::KeyInfo *>(eventQueue.events().at(i).getData());
    QCOMPARE(info->m_key, expectedKeys.at(i));
    QCOMPARE(info->m_button, expectedButtons.at(i));
  }
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
