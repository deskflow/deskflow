/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "KeyMapTests.h"

#include "deskflow/KeyMap.h"

using namespace deskflow;
using KeyItemList = KeyMap::KeyItemList;
using KeyEntryList = std::vector<KeyItemList>;

void KeyMapTests::findBestKey_requiredDown_matchExactFirstItem()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList;
  KeyMap::KeyItem item;
  item.m_required = KeyModifierShift;
  item.m_sensitive = KeyModifierShift;
  KeyModifierMask desiredState = KeyModifierShift;
  itemList.push_back(item);
  entryList.push_back(itemList);

  QCOMPARE(keyMap.findBestKey(entryList, desiredState), 0);
}

void KeyMapTests::findBestKey_requiredAndExtraSensitiveDown_matchExactFirstItem()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList;
  KeyMap::KeyItem item;
  item.m_required = KeyModifierShift;
  item.m_sensitive = KeyModifierShift | KeyModifierAlt;
  KeyModifierMask desiredState = KeyModifierShift;
  itemList.push_back(item);
  entryList.push_back(itemList);

  QCOMPARE(keyMap.findBestKey(entryList, desiredState), 0);
}

void KeyMapTests::findBestKey_requiredAndExtraSensitiveDown_matchExactSecondItem()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList1;
  KeyMap::KeyItem item1;
  item1.m_required = KeyModifierAlt;
  item1.m_sensitive = KeyModifierShift | KeyModifierAlt;
  KeyMap::KeyItemList itemList2;
  KeyMap::KeyItem item2;
  item2.m_required = KeyModifierShift;
  item2.m_sensitive = KeyModifierShift | KeyModifierAlt;
  KeyModifierMask desiredState = KeyModifierShift;
  itemList1.push_back(item1);
  itemList2.push_back(item2);
  entryList.push_back(itemList1);
  entryList.push_back(itemList2);
  QCOMPARE(keyMap.findBestKey(entryList, desiredState), 1);
}

void KeyMapTests::findBestKey_extraSensitiveDown_matchExactSecondItem()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList1;
  KeyMap::KeyItem item1;
  item1.m_required = 0;
  item1.m_sensitive = KeyModifierAlt;
  KeyMap::KeyItemList itemList2;
  KeyMap::KeyItem item2;
  item2.m_required = 0;
  item2.m_sensitive = KeyModifierShift;
  KeyModifierMask desiredState = KeyModifierAlt;
  itemList1.push_back(item1);
  itemList2.push_back(item2);
  entryList.push_back(itemList1);
  entryList.push_back(itemList2);

  QCOMPARE(keyMap.findBestKey(entryList, desiredState), 1);
}

void KeyMapTests::findBestKey_noRequiredDown_matchOneRequiredChangeItem()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList1;
  KeyMap::KeyItem item1;
  item1.m_required = KeyModifierShift | KeyModifierAlt;
  item1.m_sensitive = KeyModifierShift | KeyModifierAlt;
  KeyMap::KeyItemList itemList2;
  KeyMap::KeyItem item2;
  item2.m_required = KeyModifierShift;
  item2.m_sensitive = KeyModifierShift | KeyModifierAlt;
  KeyModifierMask desiredState = 0;
  itemList1.push_back(item1);
  itemList2.push_back(item2);
  entryList.push_back(itemList1);
  entryList.push_back(itemList2);

  QCOMPARE(keyMap.findBestKey(entryList, desiredState), 1);
}

void KeyMapTests::findBestKey_onlyOneRequiredDown_matchTwoRequiredChangesItem()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList1;
  KeyMap::KeyItem item1;
  item1.m_required = KeyModifierShift | KeyModifierAlt | KeyModifierControl;
  item1.m_sensitive = KeyModifierShift | KeyModifierAlt | KeyModifierControl;
  KeyItemList itemList2;
  KeyMap::KeyItem item2;
  item2.m_required = KeyModifierShift | KeyModifierAlt;
  item2.m_sensitive = KeyModifierShift | KeyModifierAlt | KeyModifierControl;
  KeyModifierMask desiredState = 0;
  itemList1.push_back(item1);
  itemList2.push_back(item2);
  entryList.push_back(itemList1);
  entryList.push_back(itemList2);

  QCOMPARE(keyMap.findBestKey(entryList, desiredState), 1);
}

void KeyMapTests::findBestKey_noRequiredDown_cannotMatch()
{
  KeyMap keyMap;
  KeyEntryList entryList;
  KeyItemList itemList;
  KeyMap::KeyItem item;
  item.m_required = 0xffffffff;
  item.m_sensitive = 0xffffffff;
  KeyModifierMask desiredState = 0;
  itemList.push_back(item);
  entryList.push_back(itemList);

  QCOMPARE(keyMap.findBestKey(entryList, desiredState), -1);
}

void KeyMapTests::isCommand()
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierShift;
  QVERIFY(!keyMap.isCommand(mask));

  mask = KeyModifierControl;
  QVERIFY(keyMap.isCommand(mask));

  mask = KeyModifierAlt;
  QVERIFY(keyMap.isCommand(mask));

  mask = KeyModifierAltGr;
  QVERIFY(keyMap.isCommand(mask));

  mask = KeyModifierMeta;
  QVERIFY(keyMap.isCommand(mask));

  mask = KeyModifierSuper;
  QVERIFY(keyMap.isCommand(mask));
}

void KeyMapTests::mapkey()
{
  KeyMap keyMap{};
  KeyMap::Keystroke stroke('A', true, false, 1);
  KeyMap::KeyItem keyItem;
  keyItem.m_button = 'A';
  keyItem.m_group = 1;
  keyItem.m_id = 'A';
  keyMap.addKeyEntry(keyItem);
  keyMap.finish();
  KeyMap::Keystrokes strokes{stroke};
  KeyMap::ModifierToKeys activeModifiers{};
  KeyModifierMask currentState{};
  KeyModifierMask desiredMask{};
  auto result = keyMap.mapKey(strokes, kKeySetModifiers, 1, activeModifiers, currentState, desiredMask, false, "en");
  QVERIFY(result != nullptr);
  desiredMask = KeyModifierControl;
  result = keyMap.mapKey(strokes, kKeySetModifiers, 1, activeModifiers, currentState, desiredMask, false, "en");
  QVERIFY(result == nullptr);
}

void KeyMapTests::parseModifiers_plusKey_keepsPlusAsKey()
{
  std::string keystroke = "Control+Shift++";
  KeyModifierMask mask = 0;

  QVERIFY(KeyMap::parseModifiers(keystroke, mask));
  QCOMPARE(mask, static_cast<KeyModifierMask>(KeyModifierControl | KeyModifierShift));
  QCOMPARE(keystroke, std::string("+"));
}

void KeyMapTests::parseKey_plusSymbol_parsesAsAsciiKey()
{
  KeyID key = kKeyNone;

  QVERIFY(KeyMap::parseKey("+", key));
  QCOMPARE(key, static_cast<KeyID>('+'));
}

void KeyMapTests::groupLanguages_preserveSourceIndices()
{
  KeyMap keyMap;
  keyMap.setLanguageData({"zh", "en", "fr"});
  // IME before ABC, another English layout, then French.
  keyMap.setGroupLanguageData({"", "en", "en", "fr"});

  QCOMPARE(keyMap.getLanguageGroupID(0, "en"), 1);
  QCOMPARE(keyMap.getLanguageGroupID(1, "fr"), 3);
}

void KeyMapTests::groupLanguages_keepCurrentDuplicate()
{
  KeyMap keyMap;
  keyMap.setGroupLanguageData({"en", "", "en"});

  QCOMPARE(keyMap.getLanguageGroupID(2, "en"), 2);
}

void KeyMapTests::groupLanguages_imeOnlyKeepsCurrentGroup()
{
  KeyMap keyMap;
  keyMap.setLanguageData({"en", "zh"});
  keyMap.setGroupLanguageData({"en", "", "", "fr"});

  QCOMPARE(keyMap.getLanguageGroupID(0, "zh"), 0);
  QCOMPARE(keyMap.getLanguageGroupID(3, "zh"), 3);
  QCOMPARE(keyMap.getLanguageGroupID(3, "ja"), 3);
}

void KeyMapTests::groupLanguages_emptyLanguageKeepsCurrentGroup()
{
  KeyMap keyMap;
  keyMap.setGroupLanguageData({"", "en"});

  QCOMPARE(keyMap.getLanguageGroupID(1, ""), 1);
}

void KeyMapTests::groupLanguages_followKeymapSwap()
{
  KeyMap active;
  active.setLanguageData({"en", "zh", "fr"});
  active.setGroupLanguageData({"en", "", "fr"});
  KeyMap rebuilt;
  rebuilt.setGroupLanguageData({"", "en", "en", "fr"});

  active.swap(rebuilt);

  QCOMPARE(active.getLanguageGroupID(0, "en"), 1);
  QCOMPARE(active.getLanguageGroupID(1, "fr"), 3);
  QCOMPARE(active.getLanguageGroupID(1, "zh"), 1);
  QCOMPARE(rebuilt.getLanguageGroupID(0, "fr"), 2);
}

void KeyMapTests::groupLanguages_emptyOverrideDoesNotUseInstalledList()
{
  KeyMap keyMap;
  keyMap.setLanguageData({"en", "zh"});
  keyMap.setGroupLanguageData({});

  QCOMPARE(keyMap.getLanguageGroupID(0, "zh"), 0);
}

void KeyMapTests::groupLanguages_legacyMappingIsPreserved()
{
  KeyMap keyMap;
  keyMap.setLanguageData({"en", "fr"});
  KeyMap rebuilt;
  keyMap.swap(rebuilt);

  QCOMPARE(keyMap.getLanguageGroupID(0, "fr"), 1);
  QCOMPARE(keyMap.getLanguageGroupID(1, "en"), 0);
  QCOMPARE(keyMap.getLanguageGroupID(1, "zh"), 1);
}

void KeyMapTests::mapKey_imeOnlyLanguageDoesNotSwitchGroup()
{
  KeyMap keyMap;
  keyMap.setLanguageData({"zh", "en"});
  keyMap.setGroupLanguageData({"", "en", ""});
  KeyMap::KeyItem item;
  item.m_id = 'a';
  item.m_button = 1;
  item.m_group = 1;
  keyMap.addKeyEntry(item);
  keyMap.finish();

  KeyMap::Keystrokes keys;
  KeyMap::ModifierToKeys modifiers;
  KeyModifierMask state = 0;
  auto result = keyMap.mapKey(keys, 'a', 1, modifiers, state, 0, false, "zh");

  QVERIFY(result != nullptr);
  QCOMPARE(result->m_group, 1);
  QVERIFY(!keys.empty());
  for (const auto &key : keys) {
    QVERIFY(key.m_type != KeyMap::Keystroke::KeyType::Group);
  }
}

QTEST_MAIN(KeyMapTests)
