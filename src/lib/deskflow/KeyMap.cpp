/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2005 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/KeyMap.h"
#include "base/Log.h"
#include "deskflow/App.h"
#include "deskflow/ArgsBase.h"
#include "deskflow/key_types.h"

#include <assert.h>
#include <cctype>
#include <cstdlib>

namespace deskflow {

KeyMap::NameToKeyMap *KeyMap::s_nameToKeyMap = NULL;
KeyMap::NameToModifierMap *KeyMap::s_nameToModifierMap = NULL;
KeyMap::KeyToNameMap *KeyMap::s_keyToNameMap = NULL;
KeyMap::ModifierToNameMap *KeyMap::s_modifierToNameMap = NULL;

KeyMap::KeyMap() : m_numGroups(0), m_composeAcrossGroups(false)
{
  m_modifierKeyItem.m_id = kKeyNone;
  m_modifierKeyItem.m_group = 0;
  m_modifierKeyItem.m_button = 0;
  m_modifierKeyItem.m_required = 0;
  m_modifierKeyItem.m_sensitive = 0;
  m_modifierKeyItem.m_generates = 0;
  m_modifierKeyItem.m_dead = false;
  m_modifierKeyItem.m_lock = false;
  m_modifierKeyItem.m_client = 0;
}

KeyMap::~KeyMap()
{
  // do nothing
}

void KeyMap::swap(KeyMap &x)
{
  m_keyIDMap.swap(x.m_keyIDMap);
  m_modifierKeys.swap(x.m_modifierKeys);
  m_halfDuplex.swap(x.m_halfDuplex);
  m_halfDuplexMods.swap(x.m_halfDuplexMods);
  int32_t tmp1 = m_numGroups;
  m_numGroups = x.m_numGroups;
  x.m_numGroups = tmp1;
  bool tmp2 = m_composeAcrossGroups;
  m_composeAcrossGroups = x.m_composeAcrossGroups;
  x.m_composeAcrossGroups = tmp2;
}

void KeyMap::addKeyEntry(const KeyItem &item)
{
  // ignore kKeyNone
  if (item.m_id == kKeyNone) {
    return;
  }

  // resize number of groups for key
  int32_t numGroups = item.m_group + 1;
  if (getNumGroups() > numGroups) {
    numGroups = getNumGroups();
  }
  KeyGroupTable &groupTable = m_keyIDMap[item.m_id];
  if (groupTable.size() < static_cast<size_t>(numGroups)) {
    groupTable.resize(numGroups);
  }

  // make a list from the item
  KeyItemList items;
  items.push_back(item);

  // set group and dead key flag on the item
  KeyItem &newItem = items.back();
  newItem.m_dead = isDeadKey(item.m_id);

  // mask the required bits with the sensitive bits
  newItem.m_required &= newItem.m_sensitive;

  // see if we already have this item;  just return if so
  KeyEntryList &entries = groupTable[item.m_group];
  for (size_t i = 0, n = entries.size(); i < n; ++i) {
    if (entries[i].size() == 1 && newItem == entries[i][0]) {
      return;
    }
  }

  // add item list
  entries.push_back(items);
  LOG(
      (CLOG_DEBUG5 "add key: %04x %d %03x %04x (%04x %04x %04x)%s", newItem.m_id, newItem.m_group, newItem.m_button,
       newItem.m_client, newItem.m_required, newItem.m_sensitive, newItem.m_generates, newItem.m_dead ? " dead" : "")
  );
}

void KeyMap::addKeyAliasEntry(
    KeyID targetID, int32_t group, KeyModifierMask targetRequired, KeyModifierMask targetSensitive, KeyID sourceID,
    KeyModifierMask sourceRequired, KeyModifierMask sourceSensitive
)
{
  // if we can already generate the target as desired then we're done.
  if (findCompatibleKey(targetID, group, targetRequired, targetSensitive) != NULL) {
    return;
  }

  // find a compatible source, preferably in the same group
  for (int32_t gd = 0, n = getNumGroups(); gd < n; ++gd) {
    int32_t eg = getEffectiveGroup(group, gd);
    const KeyItemList *sourceEntry = findCompatibleKey(sourceID, eg, sourceRequired, sourceSensitive);
    if (sourceEntry != NULL && sourceEntry->size() == 1) {
      KeyMap::KeyItem targetItem = sourceEntry->back();
      targetItem.m_id = targetID;
      targetItem.m_group = eg;
      addKeyEntry(targetItem);
      break;
    }
  }
}

bool KeyMap::addKeyCombinationEntry(KeyID id, int32_t group, const KeyID *keys, uint32_t numKeys)
{
  // disallow kKeyNone
  if (id == kKeyNone) {
    return false;
  }

  int32_t numGroups = group + 1;
  if (getNumGroups() > numGroups) {
    numGroups = getNumGroups();
  }
  KeyGroupTable &groupTable = m_keyIDMap[id];
  if (groupTable.size() < static_cast<size_t>(numGroups)) {
    groupTable.resize(numGroups);
  }
  if (!groupTable[group].empty()) {
    // key is already in the table
    return false;
  }

  // convert to buttons
  KeyItemList items;
  for (uint32_t i = 0; i < numKeys; ++i) {
    KeyIDMap::const_iterator gtIndex = m_keyIDMap.find(keys[i]);
    if (gtIndex == m_keyIDMap.end()) {
      return false;
    }
    const KeyGroupTable &groupTable = gtIndex->second;

    // if we allow group switching during composition then search all
    // groups for keys, otherwise search just the given group.
    int32_t n = 1;
    if (m_composeAcrossGroups) {
      n = (int32_t)groupTable.size();
    }

    bool found = false;
    for (int32_t gd = 0; gd < n && !found; ++gd) {
      int32_t eg = (group + gd) % getNumGroups();
      const KeyEntryList &entries = groupTable[eg];
      for (size_t j = 0; j < entries.size(); ++j) {
        if (entries[j].size() == 1) {
          found = true;
          items.push_back(entries[j][0]);
          break;
        }
      }
    }
    if (!found) {
      // required key is not in keyboard group
      return false;
    }
  }

  // add key
  groupTable[group].push_back(items);
  return true;
}

void KeyMap::allowGroupSwitchDuringCompose()
{
  m_composeAcrossGroups = true;
}

void KeyMap::addHalfDuplexButton(KeyButton button)
{
  m_halfDuplex.insert(button);
}

void KeyMap::clearHalfDuplexModifiers()
{
  m_halfDuplexMods.clear();
}

void KeyMap::addHalfDuplexModifier(KeyID key)
{
  m_halfDuplexMods.insert(key);
}

void KeyMap::finish()
{
  m_numGroups = findNumGroups();

  // make sure every key has the same number of groups
  for (KeyIDMap::iterator i = m_keyIDMap.begin(); i != m_keyIDMap.end(); ++i) {
    i->second.resize(m_numGroups);
  }

  // compute keys that generate each modifier
  setModifierKeys();
}

void KeyMap::foreachKey(ForeachKeyCallback cb, void *userData)
{
  for (KeyIDMap::iterator i = m_keyIDMap.begin(); i != m_keyIDMap.end(); ++i) {
    KeyGroupTable &groupTable = i->second;
    for (size_t group = 0; group < groupTable.size(); ++group) {
      KeyEntryList &entryList = groupTable[group];
      for (size_t j = 0; j < entryList.size(); ++j) {
        KeyItemList &itemList = entryList[j];
        for (size_t k = 0; k < itemList.size(); ++k) {
          (*cb)(i->first, static_cast<int32_t>(group), itemList[k], userData);
        }
      }
    }
  }
}

const KeyMap::KeyItem *KeyMap::mapKey(
    Keystrokes &keys, KeyID id, int32_t group, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    KeyModifierMask desiredMask, bool isAutoRepeat, const std::string &lang
) const
{
  LOG(
      (CLOG_DEBUG1 "mapKey %04x (%d) with mask %04x, start state: %04x, group: %d", id, id, desiredMask, currentState,
       group)
  );

  // handle group change
  if (id == kKeyNextGroup) {
    keys.push_back(Keystroke(1, false, false));
    return NULL;
  } else if (id == kKeyPrevGroup) {
    keys.push_back(Keystroke(-1, false, false));
    return NULL;
  }

  const KeyItem *item;
  switch (id) {
  case kKeyShift_L:
  case kKeyShift_R:
  case kKeyControl_L:
  case kKeyControl_R:
  case kKeyAlt_L:
  case kKeyAlt_R:
  case kKeyMeta_L:
  case kKeyMeta_R:
  case kKeySuper_L:
  case kKeySuper_R:
  case kKeyAltGr:
  case kKeyCapsLock:
  case kKeyNumLock:
  case kKeyScrollLock:
    item = mapModifierKey(keys, id, group, activeModifiers, currentState, desiredMask, isAutoRepeat, lang);
    break;

  case kKeySetModifiers:
    if (!keysForModifierState(0, group, activeModifiers, currentState, desiredMask, desiredMask, 0, keys)) {
      LOG((CLOG_DEBUG1 "unable to set modifiers %04x", desiredMask));
      return NULL;
    }
    return &m_modifierKeyItem;

  case kKeyClearModifiers:
    if (!keysForModifierState(
            0, group, activeModifiers, currentState, currentState & ~desiredMask, desiredMask, 0, keys
        )) {
      LOG((CLOG_DEBUG1 "unable to clear modifiers %04x", desiredMask));
      return NULL;
    }
    return &m_modifierKeyItem;

  default:
    if (isCommand(desiredMask)) {
      item = mapCommandKey(keys, id, group, activeModifiers, currentState, desiredMask, isAutoRepeat, lang);
    } else {
      item = mapCharacterKey(keys, id, group, activeModifiers, currentState, desiredMask, isAutoRepeat, lang);
    }
    break;
  }

  if (item != NULL) {
    LOG((CLOG_DEBUG1 "mapped to %03x, new state %04x", item->m_button, currentState));
  }
  return item;
}

void KeyMap::setLanguageData(std::vector<std::string> layouts)
{
  m_keyboardLayouts = std::move(layouts);
}

int32_t KeyMap::getLanguageGroupID(int32_t group, const std::string &lang) const
{
  int32_t id = group;

  auto it = std::find(m_keyboardLayouts.begin(), m_keyboardLayouts.end(), lang);
  if (it != m_keyboardLayouts.end()) {
    id = static_cast<int>(std::distance(m_keyboardLayouts.begin(), it));
    LOG((CLOG_DEBUG1 "language %s has group id %d", lang.c_str(), id));
  } else {
    LOG((CLOG_DEBUG1 "could not found requested language"));
  }

  return id;
}

int32_t KeyMap::getNumGroups() const
{
  return m_numGroups;
}

int32_t KeyMap::getEffectiveGroup(int32_t group, int32_t offset) const
{
  return (group + offset + getNumGroups()) % getNumGroups();
}

const KeyMap::KeyItemList *
KeyMap::findCompatibleKey(KeyID id, int32_t group, KeyModifierMask required, KeyModifierMask sensitive) const
{
  assert(group >= 0 && group < getNumGroups());

  KeyIDMap::const_iterator i = m_keyIDMap.find(id);
  if (i == m_keyIDMap.end()) {
    return NULL;
  }

  const KeyEntryList &entries = i->second[group];
  for (size_t j = 0; j < entries.size(); ++j) {
    if ((entries[j].back().m_sensitive & sensitive) == 0 ||
        (entries[j].back().m_required & sensitive) == (required & sensitive)) {
      return &entries[j];
    }
  }

  return NULL;
}

bool KeyMap::isHalfDuplex(KeyID key, KeyButton button) const
{
  return (m_halfDuplex.count(button) + m_halfDuplexMods.count(key) > 0);
}

bool KeyMap::isCommand(KeyModifierMask mask) const
{
  return ((mask & getCommandModifiers()) != 0);
}

KeyModifierMask KeyMap::getCommandModifiers() const
{
  // we currently treat ctrl, alt, meta and super as command modifiers.
  // some platforms may have a more limited set (OS X only needs Alt)
  // but this works anyway.
  return KeyModifierControl | KeyModifierAlt | KeyModifierAltGr | KeyModifierMeta | KeyModifierSuper;
}

void KeyMap::collectButtons(const ModifierToKeys &mods, ButtonToKeyMap &keys)
{
  keys.clear();
  for (ModifierToKeys::const_iterator i = mods.begin(); i != mods.end(); ++i) {
    keys.insert(std::make_pair(i->second.m_button, &i->second));
  }
}

void KeyMap::initModifierKey(KeyItem &item)
{
  item.m_generates = 0;
  item.m_lock = false;
  switch (item.m_id) {
  case kKeyShift_L:
  case kKeyShift_R:
    item.m_generates = KeyModifierShift;
    break;

  case kKeyControl_L:
  case kKeyControl_R:
    item.m_generates = KeyModifierControl;
    break;

  case kKeyAlt_L:
  case kKeyAlt_R:
    item.m_generates = KeyModifierAlt;
    break;

  case kKeyMeta_L:
  case kKeyMeta_R:
    item.m_generates = KeyModifierMeta;
    break;

  case kKeySuper_L:
  case kKeySuper_R:
    item.m_generates = KeyModifierSuper;
    break;

  case kKeyAltGr:
    item.m_generates = KeyModifierAltGr;
    break;

  case kKeyCapsLock:
    item.m_generates = KeyModifierCapsLock;
    item.m_lock = true;
    break;

  case kKeyNumLock:
    item.m_generates = KeyModifierNumLock;
    item.m_lock = true;
    break;

  case kKeyScrollLock:
    item.m_generates = KeyModifierScrollLock;
    item.m_lock = true;
    break;

  default:
    // not a modifier
    break;
  }
}

int32_t KeyMap::findNumGroups() const
{
  size_t max = 0;
  for (KeyIDMap::const_iterator i = m_keyIDMap.begin(); i != m_keyIDMap.end(); ++i) {
    if (i->second.size() > max) {
      max = i->second.size();
    }
  }
  return static_cast<int32_t>(max);
}

void KeyMap::setModifierKeys()
{
  m_modifierKeys.clear();
  m_modifierKeys.resize(kKeyModifierNumBits * getNumGroups());
  for (KeyIDMap::const_iterator i = m_keyIDMap.begin(); i != m_keyIDMap.end(); ++i) {
    const KeyGroupTable &groupTable = i->second;
    for (size_t g = 0; g < groupTable.size(); ++g) {
      const KeyEntryList &entries = groupTable[g];
      for (size_t j = 0; j < entries.size(); ++j) {
        // skip multi-key sequences
        if (entries[j].size() != 1) {
          continue;
        }

        // skip keys that don't generate a modifier
        const KeyItem &item = entries[j].back();
        if (item.m_generates == 0) {
          continue;
        }

        // add key to each indicated modifier in this group
        for (int32_t b = 0; b < kKeyModifierNumBits; ++b) {
          // skip if item doesn't generate bit b
          if (((1u << b) & item.m_generates) != 0) {
            int32_t mIndex = (int32_t)g * kKeyModifierNumBits + b;
            m_modifierKeys[mIndex].push_back(&item);
          }
        }
      }
    }
  }
}

const KeyMap::KeyItem *KeyMap::mapCommandKey(
    Keystrokes &keys, KeyID id, int32_t group, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    KeyModifierMask desiredMask, bool isAutoRepeat, const std::string &lang
) const
{
  static const KeyModifierMask s_overrideModifiers = 0xffffu;

  // find KeySym in table
  KeyIDMap::const_iterator i = m_keyIDMap.find(id);
  if (i == m_keyIDMap.end()) {
    // unknown key
    LOG((CLOG_DEBUG1 "key %04x is not on keyboard", id));
    return NULL;
  }
  const KeyGroupTable &keyGroupTable = i->second;

  // find the first key that generates this KeyID
  const KeyItem *keyItem = NULL;
  int32_t numGroups = getNumGroups();
  for (int32_t groupOffset = 0; groupOffset < numGroups; ++groupOffset) {
    int32_t effectiveGroup = getEffectiveGroup(group, groupOffset);
    const KeyEntryList &entryList = keyGroupTable[effectiveGroup];
    for (size_t i = 0; i < entryList.size(); ++i) {
      if (entryList[i].size() != 1) {
        // ignore multikey entries
        continue;
      }

      // match based on shift and make sure all required modifiers,
      // except shift, are already in the desired mask;  we're
      // after the right button not the right character.
      // we'll use desiredMask as-is, overriding the key's required
      // modifiers, when synthesizing this button.
      const KeyItem &item = entryList[i].back();
      KeyModifierMask desiredShiftMask = KeyModifierShift & desiredMask;
      KeyModifierMask requiredIgnoreShiftMask = item.m_required & ~KeyModifierShift;
      if ((item.m_required & desiredShiftMask) == (item.m_sensitive & desiredShiftMask) &&
          ((requiredIgnoreShiftMask & desiredMask) == requiredIgnoreShiftMask)) {
        LOG((CLOG_DEBUG1 "found key in group %d", effectiveGroup));
        keyItem = &item;
        break;
      }
    }
    if (keyItem != NULL) {
      break;
    }
  }
  if (keyItem == NULL) {
    // no mapping for this keysym
    LOG((CLOG_DEBUG1 "no mapping for key %04x", id));
    return NULL;
  }

  // make working copy of modifiers
  ModifierToKeys newModifiers = activeModifiers;
  KeyModifierMask newState = currentState;
  int32_t newGroup = group;

  // don't try to change CapsLock
  desiredMask = (desiredMask & ~KeyModifierCapsLock) | (currentState & KeyModifierCapsLock);

  // add the key
  if (!keysForKeyItem(
          *keyItem, newGroup, newModifiers, newState, desiredMask, s_overrideModifiers, isAutoRepeat, keys, lang
      )) {
    LOG((CLOG_DEBUG1 "can't map key"));
    keys.clear();
    return NULL;
  }

  // add keystrokes to restore modifier keys
  if (!keysToRestoreModifiers(*keyItem, group, newModifiers, newState, activeModifiers, keys)) {
    LOG((CLOG_DEBUG1 "modifiers were not restored"));
    keys.clear();
    return NULL;
  }

  // save new modifiers
  activeModifiers = newModifiers;
  currentState = newState;

  return keyItem;
}

const KeyMap::KeyItemList *
KeyMap::getKeyItemList(const KeyMap::KeyGroupTable &keyGroupTable, int32_t group, KeyModifierMask desiredMask) const
{
  const KeyItemList *itemList = nullptr;

  // find best key in any group, starting with the active group
  for (int32_t groupOffset = 0; groupOffset < getNumGroups(); ++groupOffset) {
    auto effectiveGroup = getEffectiveGroup(group, groupOffset);
    auto keyIndex = findBestKey(keyGroupTable[effectiveGroup], desiredMask);
    if (keyIndex != -1) {
      LOG((CLOG_DEBUG1 "found key in group %d", effectiveGroup));
      itemList = &keyGroupTable[effectiveGroup][keyIndex];
      break;
    }
  }

  return itemList;
}

const KeyMap::KeyItem *KeyMap::mapCharacterKey(
    Keystrokes &keys, KeyID id, int32_t group, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    KeyModifierMask desiredMask, bool isAutoRepeat, const std::string &lang
) const
{
  // find KeySym in table
  KeyIDMap::const_iterator i = m_keyIDMap.find(id);
  if (i == m_keyIDMap.end()) {
    // unknown key
    LOG((CLOG_DEBUG1 "key %04x is not on keyboard", id));

    return NULL;
  }

  // get keys to press for key
  auto itemList = getKeyItemList(i->second, getLanguageGroupID(group, lang), desiredMask);
  if (!itemList || itemList->empty()) {
    // no mapping for this keysym
    LOG((CLOG_DEBUG1 "no mapping for key %04x", id));
    return NULL;
  }

  const KeyItem &keyItem = itemList->back();

  // make working copy of modifiers
  ModifierToKeys newModifiers = activeModifiers;
  KeyModifierMask newState = currentState;
  int32_t newGroup = group;

  // add each key
  for (size_t j = 0; j < itemList->size(); ++j) {
    if (!keysForKeyItem(itemList->at(j), newGroup, newModifiers, newState, desiredMask, 0, isAutoRepeat, keys, lang)) {
      LOG((CLOG_DEBUG1 "can't map key"));
      keys.clear();
      return NULL;
    }
  }

  // add keystrokes to restore modifier keys
  if (!keysToRestoreModifiers(keyItem, group, newModifiers, newState, activeModifiers, keys)) {
    LOG((CLOG_DEBUG1 "modifiers were not restored"));
    keys.clear();
    return NULL;
  }

  // save new modifiers
  activeModifiers = newModifiers;
  currentState = newState;

  return &keyItem;
}

void KeyMap::addGroupToKeystroke(Keystrokes &keys, int32_t &group, const std::string &lang) const
{
  group = getLanguageGroupID(group, lang);
  keys.push_back(Keystroke(group, true, false));
}

const KeyMap::KeyItem *KeyMap::mapModifierKey(
    Keystrokes &keys, KeyID id, int32_t group, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    KeyModifierMask desiredMask, bool isAutoRepeat, const std::string &lang
) const
{
  return mapCharacterKey(keys, id, group, activeModifiers, currentState, desiredMask, isAutoRepeat, lang);
}

int32_t KeyMap::findBestKey(const KeyEntryList &entryList, KeyModifierMask desiredState) const
{
  // check for an item that can accommodate the desiredState exactly
  for (int32_t i = 0; i < (int32_t)entryList.size(); ++i) {
    const KeyItem &item = entryList[i].back();
    if ((item.m_required & desiredState) == item.m_required &&
        (item.m_required & desiredState) == (item.m_sensitive & desiredState)) {
      LOG((CLOG_DEBUG1 "best key index %d of %d (exact)", i + 1, entryList.size()));
      return i;
    }
  }

  // choose the item that requires the fewest modifier changes
  int32_t bestCount = 32;
  int32_t bestIndex = -1;
  for (int32_t i = 0; i < (int32_t)entryList.size(); ++i) {
    const KeyItem &item = entryList[i].back();
    KeyModifierMask change = ((item.m_required ^ desiredState) & item.m_sensitive);
    int32_t n = getNumModifiers(change);
    if (n < bestCount) {
      bestCount = n;
      bestIndex = i;
    }
  }
  if (bestIndex != -1) {
    LOG((CLOG_DEBUG1 "best key index %d of %d (%d modifiers)", bestIndex + 1, entryList.size(), bestCount));
  }

  return bestIndex;
}

const KeyMap::KeyItem *KeyMap::keyForModifier(KeyButton button, int32_t group, int32_t modifierBit) const
{
  assert(modifierBit >= 0 && modifierBit < kKeyModifierNumBits);
  assert(group >= 0 && group < getNumGroups());

  // find a key that generates the given modifier in the given group
  // but doesn't use the given button, presumably because we're trying
  // to generate a KeyID that's only bound the the given button.
  // this is important when a shift button is modified by shift;  we
  // must use the other shift button to do the shifting.
  const ModifierKeyItemList &items = m_modifierKeys[group * kKeyModifierNumBits + modifierBit];
  for (ModifierKeyItemList::const_iterator i = items.begin(); i != items.end(); ++i) {
    if ((*i)->m_button != button) {
      return (*i);
    }
  }
  return NULL;
}

bool KeyMap::keysForKeyItem(
    const KeyItem &keyItem, int32_t &group, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    KeyModifierMask desiredState, KeyModifierMask overrideModifiers, bool isAutoRepeat, Keystrokes &keystrokes,
    const std::string &lang
) const
{
  static const KeyModifierMask s_notRequiredMask = KeyModifierAltGr | KeyModifierNumLock | KeyModifierScrollLock;

  // add keystrokes to adjust the group
  if (group != keyItem.m_group) {
    group = keyItem.m_group;
    addGroupToKeystroke(keystrokes, group, lang);
  }

  EKeystroke type;
  if (keyItem.m_dead) {
    // adjust modifiers for dead key
    if (!keysForModifierState(
            keyItem.m_button, group, activeModifiers, currentState, keyItem.m_required, keyItem.m_sensitive, 0,
            keystrokes
        )) {
      LOG((CLOG_DEBUG1 "unable to match modifier state for dead key %d", keyItem.m_button));
      return false;
    }

    // press and release the dead key
    type = kKeystrokeClick;
  } else {
    // if this a command key then we don't have to match some of the
    // key's required modifiers.
    KeyModifierMask sensitive = keyItem.m_sensitive & ~overrideModifiers;

    // XXX -- must handle pressing a modifier.  in particular, if we want
    // to synthesize a KeyID on level 1 of a KeyButton that has Shift_L
    // mapped to level 0 then we must release that button if it's down
    // (in order to satisfy a shift modifier) then press a different
    // button (any other button) mapped to the shift modifier and then
    // the Shift_L button.
    // match key's required state
    LOG((CLOG_DEBUG1 "state: %04x,%04x,%04x", currentState, keyItem.m_required, sensitive));
    if (!keysForModifierState(
            keyItem.m_button, group, activeModifiers, currentState, keyItem.m_required, sensitive, 0, keystrokes
        )) {
      LOG(
          (CLOG_DEBUG1 "unable to match modifier state (%04x,%04x) for key %d", keyItem.m_required, keyItem.m_sensitive,
           keyItem.m_button)
      );
      return false;
    }

    // match desiredState as closely as possible.  we must not
    // change any modifiers in keyItem.m_sensitive.  and if the key
    // is a modifier, we don't want to change that modifier.
    LOG(
        (CLOG_DEBUG1 "desired state: %04x %04x,%04x,%04x", desiredState, currentState, keyItem.m_required,
         keyItem.m_sensitive)
    );
    if (!keysForModifierState(
            keyItem.m_button, group, activeModifiers, currentState, desiredState, ~(sensitive | keyItem.m_generates),
            s_notRequiredMask, keystrokes
        )) {
      LOG(
          (CLOG_DEBUG1 "unable to match desired modifier state (%04x,%04x) for key %d", desiredState,
           ~keyItem.m_sensitive & 0xffffu, keyItem.m_button)
      );
      return false;
    }

    // repeat or press of key
    type = isAutoRepeat ? kKeystrokeRepeat : kKeystrokePress;
  }
  addKeystrokes(type, keyItem, activeModifiers, currentState, keystrokes);

  return true;
}

bool KeyMap::keysToRestoreModifiers(
    const KeyItem &keyItem, int32_t, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    const ModifierToKeys &desiredModifiers, Keystrokes &keystrokes
) const
{
  // XXX -- we're not considering modified modifiers here

  ModifierToKeys oldModifiers = activeModifiers;

  // get the pressed modifier buttons before and after
  ButtonToKeyMap oldKeys, newKeys;
  collectButtons(oldModifiers, oldKeys);
  collectButtons(desiredModifiers, newKeys);

  // release unwanted keys
  for (ModifierToKeys::const_iterator i = oldModifiers.begin(); i != oldModifiers.end(); ++i) {
    KeyButton button = i->second.m_button;
    if (button != keyItem.m_button && newKeys.count(button) == 0) {
      EKeystroke type = kKeystrokeRelease;
      if (i->second.m_lock) {
        type = kKeystrokeUnmodify;
      }
      addKeystrokes(type, i->second, activeModifiers, currentState, keystrokes);
    }
  }

  // press wanted keys
  for (ModifierToKeys::const_iterator i = desiredModifiers.begin(); i != desiredModifiers.end(); ++i) {
    KeyButton button = i->second.m_button;
    if (button != keyItem.m_button && oldKeys.count(button) == 0) {
      EKeystroke type = kKeystrokePress;
      if (i->second.m_lock) {
        type = kKeystrokeModify;
      }
      addKeystrokes(type, i->second, activeModifiers, currentState, keystrokes);
    }
  }

  return true;
}

bool KeyMap::keysForModifierState(
    KeyButton button, int32_t group, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    KeyModifierMask requiredState, KeyModifierMask sensitiveMask, KeyModifierMask notRequiredMask,
    Keystrokes &keystrokes
) const
{
  // compute which modifiers need changing
  KeyModifierMask flipMask = ((currentState ^ requiredState) & sensitiveMask);
  // if a modifier is not required then don't even try to match it.  if
  // we don't mask out notRequiredMask then we'll try to match those
  // modifiers but succeed if we can't.  however, this is known not
  // to work if the key itself is a modifier (the numlock toggle can
  // interfere) so we don't try to match at all.
  flipMask &= ~notRequiredMask;
  LOG(
      (CLOG_DEBUG1 "flip: %04x (%04x vs %04x in %04x - %04x)", flipMask, currentState, requiredState,
       sensitiveMask & 0xffffu, notRequiredMask & 0xffffu)
  );
  if (flipMask == 0) {
    return true;
  }

  // fix modifiers.  this is complicated by the fact that a modifier may
  // be sensitive to other modifiers!  (who thought that up?)
  //
  // we'll assume that modifiers with higher bits are affected by modifiers
  // with lower bits.  there's not much basis for that assumption except
  // that we're pretty sure shift isn't changed by other modifiers.
  int32_t bit = kKeyModifierNumBits;
  while (bit-- > 0) {
    KeyModifierMask mask = (1u << bit);
    if ((flipMask & mask) == 0) {
      // modifier is already correct
      continue;
    }

    // do we want the modifier active or inactive?
    bool active = ((requiredState & mask) != 0);

    // get the KeyItem for the modifier in the group
    const KeyItem *keyItem = keyForModifier(button, group, bit);
    if (keyItem == NULL) {
      if ((mask & notRequiredMask) == 0) {
        LOG((CLOG_DEBUG1 "no key for modifier %04x", mask));
        return false;
      } else {
        continue;
      }
    }

    // if this modifier is sensitive to modifiers then adjust those
    // modifiers.  also check if our assumption was correct.  note
    // that we only need to adjust the modifiers on key down.
    KeyModifierMask sensitive = keyItem->m_sensitive;
    if ((sensitive & mask) != 0) {
      // modifier is sensitive to itself.  that makes no sense
      // so ignore it.
      LOG((CLOG_DEBUG1 "modifier %04x modified by itself", mask));
      sensitive &= ~mask;
    }
    if (sensitive != 0) {
      if (sensitive > mask) {
        // our assumption is incorrect
        LOG((CLOG_DEBUG1 "modifier %04x modified by %04x", mask, sensitive));
        return false;
      }
      if (active &&
          !keysForModifierState(
              button, group, activeModifiers, currentState, keyItem->m_required, sensitive, notRequiredMask, keystrokes
          )) {
        return false;
      } else if (!active) {
        // release the modifier
        // XXX -- this doesn't work!  if Alt and Meta are mapped
        // to one key and we want to release Meta we can't do
        // that without also releasing Alt.
        // need to think about support for modified modifiers.
      }
    }

    // current state should match required state
    if ((currentState & sensitive) != (keyItem->m_required & sensitive)) {
      LOG(
          (CLOG_DEBUG1 "unable to match modifier state for modifier %04x (%04x "
                       "vs %04x in %04x)",
           mask, currentState, keyItem->m_required, sensitive)
      );
      return false;
    }

    // add keystrokes
    EKeystroke type = active ? kKeystrokeModify : kKeystrokeUnmodify;
    addKeystrokes(type, *keyItem, activeModifiers, currentState, keystrokes);
  }

  return true;
}

void KeyMap::addKeystrokes(
    EKeystroke type, const KeyItem &keyItem, ModifierToKeys &activeModifiers, KeyModifierMask &currentState,
    Keystrokes &keystrokes
) const
{
  KeyButton button = keyItem.m_button;
  uint32_t data = keyItem.m_client;
  switch (type) {
  case kKeystrokePress:
    keystrokes.push_back(Keystroke(button, true, false, data));
    if (keyItem.m_generates != 0) {
      if (!keyItem.m_lock || (currentState & keyItem.m_generates) == 0) {
        // add modifier key and activate modifier
        activeModifiers.insert(std::make_pair(keyItem.m_generates, keyItem));
        currentState |= keyItem.m_generates;
      } else {
        // deactivate locking modifier
        activeModifiers.erase(keyItem.m_generates);
        currentState &= ~keyItem.m_generates;
      }
    }
    break;

  case kKeystrokeRelease:
    keystrokes.push_back(Keystroke(button, false, false, data));
    if (keyItem.m_generates != 0 && !keyItem.m_lock) {
      // remove key from active modifiers
      std::pair<ModifierToKeys::iterator, ModifierToKeys::iterator> range =
          activeModifiers.equal_range(keyItem.m_generates);
      for (ModifierToKeys::iterator i = range.first; i != range.second; ++i) {
        if (i->second.m_button == button) {
          activeModifiers.erase(i);
          break;
        }
      }

      // if no more keys for this modifier then deactivate modifier
      if (activeModifiers.count(keyItem.m_generates) == 0) {
        currentState &= ~keyItem.m_generates;
      }
    }
    break;

  case kKeystrokeRepeat:
    keystrokes.push_back(Keystroke(button, false, true, data));
    keystrokes.push_back(Keystroke(button, true, true, data));
    // no modifier changes on key repeat
    break;

  case kKeystrokeClick:
    keystrokes.push_back(Keystroke(button, true, false, data));
    keystrokes.push_back(Keystroke(button, false, false, data));
    // no modifier changes on key click
    break;

  case kKeystrokeModify:
  case kKeystrokeUnmodify:
    if (keyItem.m_lock) {
      // we assume there's just one button for this modifier
      if (m_halfDuplex.count(button) > 0) {
        if (type == kKeystrokeModify) {
          // turn half-duplex toggle on (press)
          keystrokes.push_back(Keystroke(button, true, false, data));
        } else {
          // turn half-duplex toggle off (release)
          keystrokes.push_back(Keystroke(button, false, false, data));
        }
      } else {
        // toggle (click)
        keystrokes.push_back(Keystroke(button, true, false, data));
        keystrokes.push_back(Keystroke(button, false, false, data));
      }
    } else if (type == kKeystrokeModify) {
      // press modifier
      keystrokes.push_back(Keystroke(button, true, false, data));
    } else {
      // release all the keys that generate the modifier that are
      // currently down
      std::pair<ModifierToKeys::const_iterator, ModifierToKeys::const_iterator> range =
          activeModifiers.equal_range(keyItem.m_generates);
      for (ModifierToKeys::const_iterator i = range.first; i != range.second; ++i) {
        keystrokes.push_back(Keystroke(i->second.m_button, false, false, i->second.m_client));
      }
    }

    if (type == kKeystrokeModify) {
      activeModifiers.insert(std::make_pair(keyItem.m_generates, keyItem));
      currentState |= keyItem.m_generates;
    } else {
      activeModifiers.erase(keyItem.m_generates);
      currentState &= ~keyItem.m_generates;
    }
    break;
  }
}

int32_t KeyMap::getNumModifiers(KeyModifierMask state)
{
  int32_t n = 0;
  for (; state != 0; state >>= 1) {
    if ((state & 1) != 0) {
      ++n;
    }
  }
  return n;
}

bool KeyMap::isDeadKey(KeyID key)
{
  return (key == kKeyCompose || (key >= 0x0300 && key <= 0x036f));
}

KeyID KeyMap::getDeadKey(KeyID key)
{
  if (isDeadKey(key)) {
    // already dead
    return key;
  }

  switch (key) {
  case '`':
    return kKeyDeadGrave;

  case '\'':
  case 0xb4u:
    return kKeyDeadAcute;

  case '^':
  case 0x2c6:
    return kKeyDeadCircumflex;

  case '~':
  case 0x2dcu:
    return kKeyDeadTilde;

  case 0xafu:
    return kKeyDeadMacron;

  case 0x2d8u:
    return kKeyDeadBreve;

  case 0x2d9u:
    return kKeyDeadAbovedot;

  case 0xa8u:
    return kKeyDeadDiaeresis;

  case 0xb0u:
  case 0x2dau:
    return kKeyDeadAbovering;

  case '\"':
  case 0x2ddu:
    return kKeyDeadDoubleacute;

  case 0x2c7u:
    return kKeyDeadCaron;

  case 0xb8u:
    return kKeyDeadCedilla;

  case 0x2dbu:
    return kKeyDeadOgonek;

  default:
    // unknown
    return kKeyNone;
  }
}

std::string KeyMap::formatKey(KeyID key, KeyModifierMask mask)
{
  // initialize tables
  initKeyNameMaps();

  std::string x;
  for (int32_t i = 0; i < kKeyModifierNumBits; ++i) {
    KeyModifierMask mod = (1u << i);
    if ((mask & mod) != 0 && s_modifierToNameMap->count(mod) > 0) {
      x += s_modifierToNameMap->find(mod)->second;
      x += "+";
    }
  }
  if (key != kKeyNone) {
    if (s_keyToNameMap->count(key) > 0) {
      x += s_keyToNameMap->find(key)->second;
    }
    // XXX -- we're assuming ASCII here
    else if (key >= 33 && key < 127) {
      x += (char)key;
    } else {
      x += deskflow::string::sprintf("\\u%04x", key);
    }
  } else if (!x.empty()) {
    // remove trailing '+'
    x.erase(x.size() - 1);
  }
  return x;
}

bool KeyMap::parseKey(const std::string &x, KeyID &key)
{
  // initialize tables
  initKeyNameMaps();

  // parse the key
  key = kKeyNone;
  if (s_nameToKeyMap->count(x) > 0) {
    key = s_nameToKeyMap->find(x)->second;
  }
  // XXX -- we're assuming ASCII encoding here
  else if (x.size() == 1) {
    if (!isgraph(x[0])) {
      // unknown key
      return false;
    }
    key = (KeyID)x[0];
  } else if (x.size() == 6 && x[0] == '\\' && x[1] == 'u') {
    // escaped unicode (\uXXXX where XXXX is a hex number)
    char *end;
    key = (KeyID)strtol(x.c_str() + 2, &end, 16);
    if (*end != '\0') {
      return false;
    }
  } else if (!x.empty()) {
    // unknown key
    return false;
  }

  return true;
}

bool KeyMap::parseModifiers(std::string &x, KeyModifierMask &mask)
{
  // initialize tables
  initKeyNameMaps();

  mask = 0;
  std::string::size_type tb = x.find_first_not_of(" \t", 0);
  while (tb != std::string::npos) {
    // get next component
    std::string::size_type te = x.find_first_of(" \t+)", tb);
    if (te == std::string::npos) {
      te = x.size();
    }
    std::string c = x.substr(tb, te - tb);
    if (c.empty()) {
      // missing component
      return false;
    }

    if (s_nameToModifierMap->count(c) > 0) {
      KeyModifierMask mod = s_nameToModifierMap->find(c)->second;
      if ((mask & mod) != 0) {
        // modifier appears twice
        return false;
      }
      mask |= mod;
    } else {
      // unknown string
      x.erase(0, tb);
      std::string::size_type tb = x.find_first_not_of(" \t");
      std::string::size_type te = x.find_last_not_of(" \t");
      if (tb == std::string::npos) {
        x = "";
      } else {
        x = x.substr(tb, te - tb + 1);
      }
      return true;
    }

    // check for '+' or end of string
    tb = x.find_first_not_of(" \t", te);
    if (tb != std::string::npos) {
      if (x[tb] != '+') {
        // expected '+'
        return false;
      }
      tb = x.find_first_not_of(" \t", tb + 1);
    }
  }

  // parsed the whole thing
  x = "";
  return true;
}

void KeyMap::initKeyNameMaps()
{
  // initialize tables
  if (s_nameToKeyMap == NULL) {
    s_nameToKeyMap = new NameToKeyMap;
    s_keyToNameMap = new KeyToNameMap;
    for (const KeyNameMapEntry *i = kKeyNameMap; i->m_name != NULL; ++i) {
      (*s_nameToKeyMap)[i->m_name] = i->m_id;
      (*s_keyToNameMap)[i->m_id] = i->m_name;
    }
  }
  if (s_nameToModifierMap == NULL) {
    s_nameToModifierMap = new NameToModifierMap;
    s_modifierToNameMap = new ModifierToNameMap;
    for (const KeyModifierNameMapEntry *i = kModifierNameMap; i->m_name != NULL; ++i) {
      (*s_nameToModifierMap)[i->m_name] = i->m_mask;
      (*s_modifierToNameMap)[i->m_mask] = i->m_name;
    }
  }
}

//
// KeyMap::KeyItem
//

bool KeyMap::KeyItem::operator==(const KeyItem &x) const
{
  return (
      m_id == x.m_id && m_group == x.m_group && m_button == x.m_button && m_required == x.m_required &&
      m_sensitive == x.m_sensitive && m_generates == x.m_generates && m_dead == x.m_dead && m_lock == x.m_lock &&
      m_client == x.m_client
  );
}

//
// KeyMap::Keystroke
//

KeyMap::Keystroke::Keystroke(KeyButton button, bool press, bool repeat, uint32_t data) : m_type(kButton)
{
  m_data.m_button.m_button = button;
  m_data.m_button.m_press = press;
  m_data.m_button.m_repeat = repeat;
  m_data.m_button.m_client = data;
}

KeyMap::Keystroke::Keystroke(int32_t group, bool absolute, bool restore) : m_type(kGroup)
{
  m_data.m_group.m_group = group;
  m_data.m_group.m_absolute = absolute;
  m_data.m_group.m_restore = restore;
}

} // namespace deskflow
