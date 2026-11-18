/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "base/EventQueue.h"
#include "platform/IOSXKeyResource.h"

#include <algorithm>

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

namespace {

// Deterministic ASCII keyboard-resource data, including the fallback layout's
// Caps+Shift uppercase table. Production code derives modifier requirements.
class ASCIIKeyResource : public IOSXKeyResource
{
public:
  explicit ASCIIKeyResource(uint32_t button = kVK_ANSI_A, bool capsShiftUppercase = true, KeyID letter = 'a')
      : m_button(button),
        m_capsShiftUppercase(capsShiftUppercase),
        m_letter(letter)
  {
  }

  bool isValid() const override
  {
    return true;
  }
  uint32_t getNumModifierCombinations() const override
  {
    return 256;
  }
  uint32_t getNumTables() const override
  {
    return 2;
  }
  uint32_t getNumButtons() const override
  {
    return m_button + 1;
  }
  uint32_t getTableForModifier(uint32_t mask) const override
  {
    const auto modifiers = mask & ((shiftKey | alphaLock) >> 8);
    if (modifiers == ((shiftKey | alphaLock) >> 8)) {
      return m_capsShiftUppercase ? 1 : 0;
    }
    return modifiers != 0 ? 1 : 0;
  }
  KeyID getKey(uint32_t table, uint32_t button) const override
  {
    if (button != m_button) {
      return kKeyNone;
    }
    return table == 0 ? m_letter : m_letter - ('a' - 'A');
  }

private:
  uint32_t m_button;
  bool m_capsShiftUppercase;
  KeyID m_letter;
};

// This key changes to a different letter when shifted. It is not an ASCII
// case pair and must retain ordinary character matching.
class ChangedLetterKeyResource : public ASCIIKeyResource
{
public:
  KeyID getKey(uint32_t table, uint32_t button) const override
  {
    const auto id = ASCIIKeyResource::getKey(table, button);
    return id != kKeyNone && table != 0 ? 'B' : id;
  }
};

class SymbolKeyResource : public ASCIIKeyResource
{
public:
  KeyID getKey(uint32_t table, uint32_t button) const override
  {
    const auto id = ASCIIKeyResource::getKey(table, button);
    return id == kKeyNone ? id : (table == 0 ? '1' : '!');
  }
};

class RecordingOSXKeyState : public OSXKeyState
{
public:
  RecordingOSXKeyState(
      IEventQueue *events, deskflow::KeyMap &map, const IOSXKeyResource &resource,
      InputModeQuery inputModeQuery = [] { return true; }, const IOSXKeyResource *secondResource = nullptr
  )
      : OSXKeyState(
            events, map, secondResource ? std::vector<std::string>{"en", "fr"} : std::vector<std::string>{"en"}, true,
            std::move(inputModeQuery)
        ),
        m_resource(resource),
        m_pSecondResource(secondResource)
  {
  }

  int32_t pollActiveGroup() const override
  {
    return m_group;
  }

  struct Letter
  {
    KeyModifierMask m_modifiers;
    bool m_press;
    bool m_repeat;
    KeyButton m_button;
  };
  std::vector<Letter> m_letters;
  std::vector<Keystroke> m_strokes;
  KeyModifierMask m_modifiers = 0;

protected:
  void getKeyMap(deskflow::KeyMap &map) override
  {
    getKeyMapForSpecialKeys(map, 0);
    OSXKeyState::getKeyMap(map, 0, m_resource);
    if (m_pSecondResource) {
      getKeyMapForSpecialKeys(map, 1);
      OSXKeyState::getKeyMap(map, 1, *m_pSecondResource);
    }
  }

  void fakeKey(const Keystroke &key) override
  {
    m_strokes.push_back(key);
    if (key.m_type == Keystroke::KeyType::Group) {
      m_group = key.m_data.m_group.m_group;
      return;
    }
    if (key.m_type != Keystroke::KeyType::Button) {
      return;
    }
    const auto &button = key.m_data.m_button;
    KeyModifierMask modifier = 0;
    switch (button.m_button) {
    case kVK_Shift + 1:
      modifier = KeyModifierShift;
      break;
    case kVK_Control + 1:
      modifier = KeyModifierControl;
      break;
    case kVK_Command + 1:
      modifier = KeyModifierSuper;
      break;
    case kVK_Option + 1:
      modifier = KeyModifierAlt;
      break;
    case kVK_CapsLock + 1:
      modifier = KeyModifierCapsLock;
      break;
    }
    if (modifier == 0) {
      m_letters.push_back({m_modifiers, button.m_press, button.m_repeat, button.m_button});
    }
    if (modifier) {
      m_modifiers = button.m_press ? m_modifiers | modifier : m_modifiers & ~modifier;
    }
  }

private:
  const IOSXKeyResource &m_resource;
  const IOSXKeyResource *m_pSecondResource;
  int32_t m_group = 0;
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

void OSXKeyStateTests::mapModifiersFromCarbon_carbonMask_returnsDeskflowMask_data()
{
  QTest::addColumn<uint32_t>("carbonMask");
  QTest::addColumn<KeyModifierMask>("expectedMask");
  QTest::newRow("none") << uint32_t(0) << KeyModifierMask(0);
  QTest::newRow("caps") << uint32_t(alphaLock) << KeyModifierMask(KeyModifierCapsLock);
  QTest::newRow("left-shift") << uint32_t(shiftKey) << KeyModifierMask(KeyModifierShift);
  QTest::newRow("right-shift") << uint32_t(rightShiftKey) << KeyModifierMask(KeyModifierShift);
  QTest::newRow("both-shifts") << uint32_t(shiftKey | rightShiftKey) << KeyModifierMask(KeyModifierShift);
  QTest::newRow("left-control") << uint32_t(controlKey) << KeyModifierMask(KeyModifierControl);
  QTest::newRow("right-control") << uint32_t(rightControlKey) << KeyModifierMask(KeyModifierControl);
  QTest::newRow("both-controls") << uint32_t(controlKey | rightControlKey) << KeyModifierMask(KeyModifierControl);
  QTest::newRow("left-option") << uint32_t(optionKey) << KeyModifierMask(KeyModifierAlt);
  QTest::newRow("right-option") << uint32_t(rightOptionKey) << KeyModifierMask(KeyModifierAlt);
  QTest::newRow("both-options") << uint32_t(optionKey | rightOptionKey) << KeyModifierMask(KeyModifierAlt);
  QTest::newRow("command") << uint32_t(cmdKey) << KeyModifierMask(KeyModifierSuper);
  QTest::newRow("caps-shift-option") << uint32_t(alphaLock | shiftKey | optionKey)
                                     << KeyModifierMask(KeyModifierCapsLock | KeyModifierShift | KeyModifierAlt);
}

void OSXKeyStateTests::mapModifiersFromCarbon_carbonMask_returnsDeskflowMask()
{
  QFETCH(uint32_t, carbonMask);
  QFETCH(KeyModifierMask, expectedMask);

  const auto mask = OSXKeyState::mapModifiersFromCarbon(carbonMask);

  QCOMPARE(mask, expectedMask);
}

void OSXKeyStateTests::getKeyMap_asciiResource_preservesCarbonModifierRequirements_data()
{
  QTest::addColumn<KeyID>("id");
  QTest::addColumn<KeyModifierMask>("required");
  QTest::newRow("lowercase") << KeyID('a') << KeyModifierMask(0);
  QTest::newRow("uppercase-shift") << KeyID('A') << KeyModifierMask(KeyModifierShift);
  QTest::newRow("uppercase-caps") << KeyID('A') << KeyModifierMask(KeyModifierCapsLock);
  QTest::newRow("uppercase-caps-shift") << KeyID('A') << KeyModifierMask(KeyModifierCapsLock | KeyModifierShift);
}

void OSXKeyStateTests::getKeyMap_asciiResource_preservesCarbonModifierRequirements()
{
  QFETCH(KeyID, id);
  QFETCH(KeyModifierMask, required);
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);

  state.updateKeyMap();
  const auto *entry = map.findCompatibleKey(id, 0, required, KeyModifierShift | KeyModifierCapsLock);

  QVERIFY(entry);
  QCOMPARE(entry->size(), 1u);
  QCOMPARE(entry->front().m_button, KeyButton(kVK_ANSI_A + 1));
  QCOMPARE(entry->front().m_sensitive, KeyModifierShift | KeyModifierCapsLock);
  QCOMPARE(entry->front().m_required, required);
}

void OSXKeyStateTests::fakeKeyDown_zhuyinCapsSequence_preservesCapsAndShift()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();

  state.fakeKeyDown(kKeyCapsLock, KeyModifierCapsLock, 0x3a, "zh");
  state.fakeKeyUp(0x3a);
  state.fakeKeyDown('A', KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyUp(0x1e);
  state.fakeKeyDown(kKeyShift_L, KeyModifierCapsLock | KeyModifierShift, 0x2a, "zh");
  state.fakeKeyDown('a', KeyModifierCapsLock | KeyModifierShift, 0x1e, "zh");
  state.fakeKeyUp(0x1e);
  state.fakeKeyUp(0x2a);
  state.fakeKeyDown('A', KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyUp(0x1e);

  QCOMPARE(state.m_letters.size(), 6u);
  QCOMPARE(state.m_letters[0].m_modifiers, KeyModifierCapsLock);
  QCOMPARE(state.m_letters[2].m_modifiers, KeyModifierCapsLock | KeyModifierShift);
  QCOMPARE(state.m_letters[4].m_modifiers, KeyModifierCapsLock);
  QCOMPARE(state.m_modifiers, KeyModifierCapsLock);
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
  QCOMPARE(state.getKeyState(kVK_ANSI_A + 1), 0);
  QCOMPARE(state.getKeyState(kVK_Shift + 1), 0);
}

void OSXKeyStateTests::fakeKeyDown_zhuyinCapsOff_clearsCapsModifier()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  state.fakeKeyDown(kKeyCapsLock, KeyModifierCapsLock, 0x3a, "zh");
  state.fakeKeyUp(0x3a);
  state.fakeKeyDown('A', KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyUp(0x1e);

  state.fakeKeyDown(kKeyCapsLock, 0, 0x3a, "zh");
  state.fakeKeyUp(0x3a);
  const auto capsEvents = std::count_if(state.m_strokes.begin(), state.m_strokes.end(), [](const auto &key) {
    return key.m_type == deskflow::KeyMap::Keystroke::KeyType::Button &&
           key.m_data.m_button.m_button == kVK_CapsLock + 1;
  });

  QCOMPARE(state.getActiveModifiers(), 0u);
  QCOMPARE(state.m_modifiers, 0u);
  QCOMPARE(capsEvents, 2);
}

void OSXKeyStateTests::fakeKeyDown_zhuyinCapsOffLetter_keepsModifiersClear()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  state.fakeKeyDown(kKeyCapsLock, KeyModifierCapsLock, 0x3a, "zh");
  state.fakeKeyDown(kKeyCapsLock, 0, 0x3a, "zh");

  state.fakeKeyDown('a', 0, 0x1e, "zh");
  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.m_letters.size(), 2u);
  QCOMPARE(state.m_letters.front().m_modifiers, 0u);
  QCOMPARE(state.getActiveModifiers(), 0u);
  QCOMPARE(state.getKeyState(kVK_ANSI_A + 1), 0);
}

void OSXKeyStateTests::fakeKeyRepeat_shiftChanges_preservesCapsMode()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  state.fakeKeyDown(kKeyCapsLock, KeyModifierCapsLock, 0x3a, "zh");
  state.fakeKeyUp(0x3a);
  state.fakeKeyDown('A', KeyModifierCapsLock, 0x1e, "zh");

  const auto plainRepeat = state.fakeKeyRepeat('A', KeyModifierCapsLock, 2, 0x1e, "zh");
  state.fakeKeyDown(kKeyShift_L, KeyModifierCapsLock | KeyModifierShift, 0x2a, "zh");
  const auto shiftRepeat = state.fakeKeyRepeat('a', KeyModifierCapsLock | KeyModifierShift, 1, 0x1e, "zh");
  state.fakeKeyUp(0x2a);
  const auto finalRepeat = state.fakeKeyRepeat('A', KeyModifierCapsLock, 1, 0x1e, "zh");

  QVERIFY(plainRepeat && shiftRepeat && finalRepeat);
  QCOMPARE(state.m_letters.size(), 5u);
  for (size_t i = 0; i < state.m_letters.size(); ++i) {
    QCOMPARE(state.m_letters[i].m_repeat, i != 0);
    QCOMPARE(state.m_letters[i].m_modifiers, i == 3 ? KeyModifierCapsLock | KeyModifierShift : KeyModifierCapsLock);
  }
  QCOMPARE(state.getKeyState(kVK_ANSI_A + 1), 1);
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
}

void OSXKeyStateTests::fakeKeyUp_repeatedZhuyinLetter_releasesKey()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  state.fakeKeyDown('A', KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyRepeat('A', KeyModifierCapsLock, 2, 0x1e, "zh");

  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.getKeyState(kVK_ANSI_A + 1), 0);
  QCOMPARE(state.m_letters.size(), 4u);
  QVERIFY(!state.m_letters.back().m_press);
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
}

void OSXKeyStateTests::fakeKeyRepeat_releasedZhuyinLetter_rejectsRepeat()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  state.fakeKeyDown('A', KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyUp(0x1e);
  state.m_strokes.clear();

  const auto repeated = state.fakeKeyRepeat('A', KeyModifierCapsLock, 1, 0x1e, "zh");

  QVERIFY(!repeated);
  QVERIFY(state.m_strokes.empty());
  QCOMPARE(state.getKeyState(kVK_ANSI_A + 1), 0);
}

void OSXKeyStateTests::fakeKeyDown_inputModeChanges_usesCurrentCharacterPolicy()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  bool zhuyin = true;
  RecordingOSXKeyState state(&queue, map, resource, [&zhuyin] { return zhuyin; });
  state.updateKeyMap();
  state.fakeKeyDown(kKeyCapsLock, KeyModifierCapsLock, 0x3a, "zh");
  state.fakeKeyUp(0x3a);
  state.fakeKeyDown(kKeyShift_L, KeyModifierCapsLock | KeyModifierShift, 0x2a, "zh");

  // The injected input-source query changes without rebuilding key state.
  for (bool inputMode : {true, false, true}) {
    zhuyin = inputMode;
    state.fakeKeyDown('a', KeyModifierCapsLock | KeyModifierShift, 0x1e, "zh");
    state.fakeKeyUp(0x1e);
  }
  state.fakeKeyUp(0x2a);

  QCOMPARE(state.m_letters.size(), 6u);
  QCOMPARE(state.m_letters[0].m_modifiers, KeyModifierCapsLock | KeyModifierShift);
  QCOMPARE(state.m_letters[2].m_modifiers, 0u);
  QCOMPARE(state.m_letters[4].m_modifiers, KeyModifierCapsLock | KeyModifierShift);
  QCOMPARE(state.getKeyState(kVK_ANSI_A + 1), 0);
  QCOMPARE(state.getKeyState(kVK_Shift + 1), 0);
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
}

void OSXKeyStateTests::fakeKeyDown_asciiInput_usesCharacterMapping_data()
{
  QTest::addColumn<KeyID>("id");
  QTest::addColumn<KeyModifierMask>("mask");
  QTest::addColumn<KeyModifierMask>("expected");
  QTest::newRow("uppercase") << KeyID('A') << KeyModifierMask(KeyModifierCapsLock)
                             << KeyModifierMask(KeyModifierCapsLock);
  QTest::newRow("lowercase") << KeyID('a') << KeyModifierMask(KeyModifierCapsLock | KeyModifierShift)
                             << KeyModifierMask(0);
}

void OSXKeyStateTests::fakeKeyDown_asciiInput_usesCharacterMapping()
{
  QFETCH(KeyID, id);
  QFETCH(KeyModifierMask, mask);
  QFETCH(KeyModifierMask, expected);
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource, [] { return false; });
  state.updateKeyMap();

  state.fakeKeyDown(id, mask, 0x1e, "en");
  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.m_letters.size(), 2u);
  QCOMPARE(state.m_letters.front().m_modifiers, expected);
  QCOMPARE(state.getActiveModifiers(), 0u);
}

void OSXKeyStateTests::fakeKeyDown_zhuyinLayoutSequence_preservesPhysicalKeyAndModifiers_data()
{
  QTest::addColumn<uint32_t>("button");
  QTest::addColumn<bool>("capsShiftUppercase");
  QTest::addColumn<KeyID>("letter");
  QTest::newRow("or-layout-a") << uint32_t(kVK_ANSI_A) << true << KeyID('a');
  QTest::newRow("xor-layout-a") << uint32_t(kVK_ANSI_A) << false << KeyID('a');
  QTest::newRow("or-layout-rearranged-q") << uint32_t(kVK_ANSI_Z) << true << KeyID('q');
  QTest::newRow("xor-layout-rearranged-q") << uint32_t(kVK_ANSI_Z) << false << KeyID('q');
}

void OSXKeyStateTests::fakeKeyDown_zhuyinLayoutSequence_preservesPhysicalKeyAndModifiers()
{
  QFETCH(uint32_t, button);
  QFETCH(bool, capsShiftUppercase);
  QFETCH(KeyID, letter);
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource(button, capsShiftUppercase, letter);
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  const auto upper = letter - ('a' - 'A');

  state.fakeKeyDown(upper, KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyUp(0x1e);
  state.fakeKeyDown(kKeyShift_L, KeyModifierCapsLock | KeyModifierShift, 0x2a, "zh");
  state.fakeKeyDown(letter, KeyModifierCapsLock | KeyModifierShift, 0x1e, "zh");
  state.fakeKeyUp(0x1e);
  state.fakeKeyUp(0x2a);
  state.fakeKeyDown(upper, KeyModifierCapsLock, 0x1e, "zh");
  state.fakeKeyUp(0x1e);

  QCOMPARE(state.m_letters.size(), 6u);
  for (size_t i = 0; i < state.m_letters.size(); ++i) {
    QCOMPARE(state.m_letters[i].m_button, KeyButton(button + 1));
    QCOMPARE(
        state.m_letters[i].m_modifiers, i == 2 || i == 3 ? KeyModifierCapsLock | KeyModifierShift : KeyModifierCapsLock
    );
  }
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
}

void OSXKeyStateTests::fakeKeyDown_languageTargetsOtherGroup_usesOrdinaryMapping()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource activeResource;
  ASCIIKeyResource otherResource(kVK_ANSI_Q, false);
  RecordingOSXKeyState state(&queue, map, activeResource, [] { return true; }, &otherResource);
  state.updateKeyMap();

  state.fakeKeyDown('a', KeyModifierCapsLock | KeyModifierShift, 0x1e, "fr");
  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.pollActiveGroup(), 1);
  QCOMPARE(state.m_letters.size(), 2u);
  QCOMPARE(state.m_letters.front().m_button, KeyButton(kVK_ANSI_Q + 1));
  QCOMPARE(state.m_letters.front().m_modifiers, KeyModifierCapsLock | KeyModifierShift);
}

void OSXKeyStateTests::fakeKeyDown_zhuyinShortcut_usesOrdinaryMapping_data()
{
  QTest::addColumn<KeyModifierMask>("shortcut");
  QTest::newRow("control") << KeyModifierMask(KeyModifierControl);
  QTest::newRow("command") << KeyModifierMask(KeyModifierSuper);
  QTest::newRow("option") << KeyModifierMask(KeyModifierAlt);
}

void OSXKeyStateTests::fakeKeyDown_zhuyinShortcut_usesOrdinaryMapping()
{
  QFETCH(KeyModifierMask, shortcut);
  deskflow::KeyMap map;
  EventQueue queue;
  ASCIIKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();
  state.fakeKeyDown(kKeyCapsLock, KeyModifierCapsLock, 0x3a, "zh");
  state.fakeKeyUp(0x3a);

  state.fakeKeyDown('a', KeyModifierCapsLock | shortcut, 0x1e, "zh");
  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.m_letters.size(), 2u);
  QCOMPARE(state.m_letters.front().m_modifiers, KeyModifierCapsLock | shortcut);
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
}

void OSXKeyStateTests::fakeKeyDown_nonCasePairResource_usesOrdinaryMapping()
{
  deskflow::KeyMap map;
  EventQueue queue;
  ChangedLetterKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();

  state.fakeKeyDown('a', KeyModifierCapsLock | KeyModifierShift, 0x1e, "zh");
  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.m_letters.size(), 2u);
  QCOMPARE(state.m_letters.front().m_modifiers, 0u);
}

void OSXKeyStateTests::fakeKeyDown_nonLetter_usesOrdinaryMapping()
{
  deskflow::KeyMap map;
  EventQueue queue;
  SymbolKeyResource resource;
  RecordingOSXKeyState state(&queue, map, resource);
  state.updateKeyMap();

  state.fakeKeyDown('1', KeyModifierCapsLock | KeyModifierShift, 0x1e, "zh");
  const auto released = state.fakeKeyUp(0x1e);

  QVERIFY(released);
  QCOMPARE(state.m_letters.size(), 2u);
  QCOMPARE(state.m_letters.front().m_modifiers, 0u);
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
