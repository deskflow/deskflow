/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyStateTests.h"
#include "base/EventQueue.h"
#include "deskflow/KeyMap.h"

#include "MockEventQueue.h"
#include "MockKeyMap.h"
#include "MockKeyState.h"

#include <algorithm>

namespace {

//! KeyState that records the keystrokes it is asked to synthesize.
class RecordingKeyState : public KeyState
{
public:
  RecordingKeyState(IEventQueue *events, deskflow::KeyMap &keyMap, std::vector<std::string> layouts, bool langSync)
      : KeyState(events, keyMap, std::move(layouts), langSync)
  {
  }

  int32_t pollActiveGroup() const override
  {
    return m_activeGroup;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }
  bool fakeCtrlAltDel() override
  {
    return false;
  }
  void getKeyMap(deskflow::KeyMap &) override
  {
  }
  bool fakeMediaKey(KeyID) override
  {
    return false;
  }
  void pollPressedKeys(KeyButtonSet &) const override
  {
  }
  void fakeKey(const Keystroke &keystroke) override
  {
    m_faked.push_back(keystroke);
  }

  int countStrokes(Keystroke::KeyType type) const
  {
    return static_cast<int>(std::count_if(m_faked.begin(), m_faked.end(), [type](const Keystroke &k) {
      return k.m_type == type;
    }));
  }

  int32_t m_activeGroup = 0;
  std::vector<Keystroke> m_faked;
};

//! Two groups on one button: a latin key in group 0 ("en"), a thai key in group 1 ("th").
void buildTwoGroupKeyMap(deskflow::KeyMap &keyMap, KeyID enKey, KeyID thKey, KeyButton button)
{
  deskflow::KeyMap::KeyItem item;
  item.m_button = button;

  item.m_id = enKey;
  item.m_group = 0;
  keyMap.addKeyEntry(item);

  item.m_id = thKey;
  item.m_group = 1;
  keyMap.addKeyEntry(item);

  keyMap.finish();
}

constexpr KeyID kLatinA = 'a';
constexpr KeyID kThaiFoFan = 0x0e1f; // ฟ, the same physical key as 'a' on a thai layout
constexpr KeyButton kSharedButton = 1;

} // namespace

void KeyStateTests::initTestCase()
{
  m_arch.init();
}

void KeyStateTests::keyDown()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  keyState.onKey(1, true, KeyModifierAlt);

  QVERIFY(keyState.getKeyState(1));
}

void KeyStateTests::keyUp()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);
  QVERIFY(!keyState.getKeyState(1));
}

void KeyStateTests::invalidKey()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(0, true, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(0));
}

void KeyStateTests::onKey_aKeyDown_keyStateOne()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(1, true, KeyModifierAlt);

  QVERIFY(keyState.getKeyState(1));
}

void KeyStateTests::onKey_aKeyUp_keyStateZero()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(1, false, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(1));
}

void KeyStateTests::onKey_invalidKey_keyStateZero()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(0, true, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(0));
}

void KeyStateTests::updateKeyState_pollDoesNothing_keyNotSet()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.updateKeyState();

  QVERIFY(!keyState.isKeyDown(1));
}

void KeyStateTests::updateKeyState_activeModifiers_maskNotSet()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.updateKeyState();

  QCOMPARE(0, keyState.getActiveModifiers());
}

void KeyStateTests::fakeKeyRepeat_invalidKey_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.fakeKeyRepeat(0, 0, 0, 0, "en"));
}

void KeyStateTests::fakeKeyUp_buttonNotDown_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.fakeKeyUp(0));
}

void KeyStateTests::isKeyDown_noKeysDown_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.isKeyDown(1));
}

void KeyStateTests::isKeyDown_keyDown_retrunsTrue()
{
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  deskflow::KeyMap::KeyItem key;
  key.m_button = 1;
  keyState.fakeKeyDown(1, 0, 1, "en");

  QVERIFY(keyState.isKeyDown(1));
}

void KeyStateTests::updateKeyState_pollInsertsSingleKey_keyIsDown()
{
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  deskflow::KeyMap::KeyItem key;
  key.m_button = 1;
  keyState.fakeKeyDown(1, 0, 1, "en");

  keyState.updateKeyState();
  QVERIFY(keyState.isKeyDown(1));
}

void KeyStateTests::fakeKeyDown_langSyncEnabled_switchesToServerGroup()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);

  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, true);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  QCOMPARE(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 1);
  const auto group = std::find_if(keyState.m_faked.begin(), keyState.m_faked.end(), [](const auto &k) {
    return k.m_type == deskflow::KeyMap::Keystroke::KeyType::Group;
  });
  QCOMPARE(group->m_data.m_group.m_group, 1);
}

void KeyStateTests::fakeKeyDown_langSyncDisabled_keepsLocalGroup()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);

  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, false);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  // the local layout must be left alone, but the key itself is still pressed by position
  QCOMPARE(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 0);
  QVERIFY(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Button) > 0);
  QCOMPARE(keyState.m_faked.front().m_data.m_button.m_button, kSharedButton);
}

void KeyStateTests::fakeKeyDown_altGrKeyOnTwoKeys_picksMatchingShiftLevel()
{
  // Czech layout (#10090): '<' is AltGr+Shift on keycode 52 (AltGr alone gives degree) and AltGr on keycode 59
  constexpr KeyButton kAltGrKey = 92;
  constexpr KeyButton kKeyY = 52;
  constexpr KeyButton kKeyComma = 59;
  constexpr KeyModifierMask kLevels = KeyModifierShift | KeyModifierAltGr;
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  deskflow::KeyMap::KeyItem item;
  const auto add = [&keyMap, &item](KeyButton button, KeyID id, KeyModifierMask required) {
    item.m_button = button;
    item.m_id = id;
    item.m_required = required;
    item.m_sensitive = kLevels;
    keyMap.addKeyEntry(item);
  };
  item.m_button = kAltGrKey;
  item.m_id = kKeyAltGr;
  item.m_generates = KeyModifierAltGr;
  keyMap.addKeyEntry(item);
  item.m_generates = 0;
  add(kKeyY, 'y', 0);
  add(kKeyY, 'Y', KeyModifierShift);
  add(kKeyY, 0x00b0, KeyModifierAltGr);
  add(kKeyY, '<', kLevels);
  add(kKeyComma, ',', 0);
  add(kKeyComma, '?', KeyModifierShift);
  add(kKeyComma, '<', KeyModifierAltGr);
  keyMap.finish();

  RecordingKeyState keyState(&eventQueue, keyMap, {"en"}, false);
  keyState.updateKeyMap(&keyMap);
  keyState.fakeKeyDown(kKeyAltGr, 0, kAltGrKey, "en");
  keyState.m_faked.clear();
  keyState.fakeKeyDown('<', KeyModifierAltGr, kKeyComma, "en");

  QVERIFY(!keyState.m_faked.empty());
  QCOMPARE(keyState.m_faked.back().m_data.m_button.m_button, kKeyComma);
}

void KeyStateTests::fakeKeyDown_commandKeyOnlyShifted_stillMapsButton()
{
  // Ctrl+'?' without Shift: '?' only exists shifted, so the shifted key is still the one to press
  static constexpr KeyButton kKeySlash = 61;
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  deskflow::KeyMap::KeyItem item;
  item.m_button = 37;
  item.m_id = kKeyControl_L;
  item.m_generates = KeyModifierControl;
  keyMap.addKeyEntry(item);
  item.m_button = 50;
  item.m_id = kKeyShift_L;
  item.m_generates = KeyModifierShift;
  keyMap.addKeyEntry(item);
  item.m_generates = 0;
  item.m_button = kKeySlash;
  item.m_sensitive = KeyModifierShift;
  item.m_id = '/';
  keyMap.addKeyEntry(item);
  item.m_id = '?';
  item.m_required = KeyModifierShift;
  keyMap.addKeyEntry(item);
  keyMap.finish();

  RecordingKeyState keyState(&eventQueue, keyMap, {"en"}, false);
  keyState.updateKeyMap(&keyMap);
  keyState.fakeKeyDown('?', KeyModifierControl, kKeySlash, "en");

  QVERIFY(std::any_of(keyState.m_faked.begin(), keyState.m_faked.end(), [](const deskflow::KeyMap::Keystroke &k) {
    return k.m_type == deskflow::KeyMap::Keystroke::KeyType::Button && k.m_data.m_button.m_button == kKeySlash &&
           k.m_data.m_button.m_press;
  }));
}

QTEST_MAIN(KeyStateTests)
