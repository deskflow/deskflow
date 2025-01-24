/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#define TEST_ENV

#include "deskflow/KeyMap.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

namespace deskflow {

TEST(KeyMapTests, findBestKey_requiredDown_matchExactFirstItem)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList;
  KeyMap::KeyItem item;
  item.m_required = KeyModifierShift;
  item.m_sensitive = KeyModifierShift;
  KeyModifierMask desiredState = KeyModifierShift;
  itemList.push_back(item);
  entryList.push_back(itemList);

  EXPECT_EQ(0, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, findBestKey_requiredAndExtraSensitiveDown_matchExactFirstItem)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList;
  KeyMap::KeyItem item;
  item.m_required = KeyModifierShift;
  item.m_sensitive = KeyModifierShift | KeyModifierAlt;
  KeyModifierMask desiredState = KeyModifierShift;
  itemList.push_back(item);
  entryList.push_back(itemList);

  EXPECT_EQ(0, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, findBestKey_requiredAndExtraSensitiveDown_matchExactSecondItem)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList1;
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

  EXPECT_EQ(1, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, findBestKey_extraSensitiveDown_matchExactSecondItem)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList1;
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

  EXPECT_EQ(1, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, findBestKey_noRequiredDown_matchOneRequiredChangeItem)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList1;
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

  EXPECT_EQ(1, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, findBestKey_onlyOneRequiredDown_matchTwoRequiredChangesItem)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList1;
  KeyMap::KeyItem item1;
  item1.m_required = KeyModifierShift | KeyModifierAlt | KeyModifierControl;
  item1.m_sensitive = KeyModifierShift | KeyModifierAlt | KeyModifierControl;
  KeyMap::KeyItemList itemList2;
  KeyMap::KeyItem item2;
  item2.m_required = KeyModifierShift | KeyModifierAlt;
  item2.m_sensitive = KeyModifierShift | KeyModifierAlt | KeyModifierControl;
  KeyModifierMask desiredState = 0;
  itemList1.push_back(item1);
  itemList2.push_back(item2);
  entryList.push_back(itemList1);
  entryList.push_back(itemList2);

  EXPECT_EQ(1, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, findBestKey_noRequiredDown_cannotMatch)
{
  KeyMap keyMap;
  KeyMap::KeyEntryList entryList;
  KeyMap::KeyItemList itemList;
  KeyMap::KeyItem item;
  item.m_required = 0xffffffff;
  item.m_sensitive = 0xffffffff;
  KeyModifierMask desiredState = 0;
  itemList.push_back(item);
  entryList.push_back(itemList);

  EXPECT_EQ(-1, keyMap.findBestKey(entryList, desiredState));
}

TEST(KeyMapTests, isCommand_shiftMask_returnFalse)
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierShift;

  EXPECT_FALSE(keyMap.isCommand(mask));
}

TEST(KeyMapTests, isCommand_controlMask_returnTrue)
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierControl;

  EXPECT_EQ(true, keyMap.isCommand(mask));
}

TEST(KeyMapTests, isCommand_alternateMask_returnTrue)
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierAlt;

  EXPECT_EQ(true, keyMap.isCommand(mask));
}

TEST(KeyMapTests, isCommand_alternateGraphicMask_returnTrue)
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierAltGr;

  EXPECT_EQ(true, keyMap.isCommand(mask));
}

TEST(KeyMapTests, isCommand_metaMask_returnTrue)
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierMeta;

  EXPECT_EQ(true, keyMap.isCommand(mask));
}

TEST(KeyMapTests, isCommand_superMask_returnTrue)
{
  KeyMap keyMap;
  KeyModifierMask mask = KeyModifierSuper;

  EXPECT_EQ(true, keyMap.isCommand(mask));
}

TEST(KeyMapTests, mapkey_handles_setmodifier_with_no_mapped)
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
  EXPECT_FALSE(result == nullptr);
  desiredMask = KeyModifierControl;
  result = keyMap.mapKey(strokes, kKeySetModifiers, 1, activeModifiers, currentState, desiredMask, false, "en");
  EXPECT_TRUE(result == nullptr);
}

} // namespace deskflow
