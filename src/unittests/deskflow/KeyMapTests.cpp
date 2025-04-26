/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "KeyMapTests.h"

#include "../../lib/deskflow/KeyMap.h"

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

QTEST_MAIN(KeyMapTests)
