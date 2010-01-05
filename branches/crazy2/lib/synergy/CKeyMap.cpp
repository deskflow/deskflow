/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2005 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CKeyMap.h"
#include "KeyTypes.h"
#include "CLog.h"
#include <assert.h>
#include <cctype>
#include <cstdlib>

CKeyMap::CNameToKeyMap*			CKeyMap::s_nameToKeyMap      = NULL;
CKeyMap::CNameToModifierMap*	CKeyMap::s_nameToModifierMap = NULL;
CKeyMap::CKeyToNameMap*			CKeyMap::s_keyToNameMap      = NULL;
CKeyMap::CModifierToNameMap*	CKeyMap::s_modifierToNameMap = NULL;

CKeyMap::CKeyMap()
{
	m_modifierKeyItem.m_id        = kKeyNone;
	m_modifierKeyItem.m_button    = 0;
	m_modifierKeyItem.m_required  = 0;
	m_modifierKeyItem.m_sensitive = 0;
	m_modifierKeyItem.m_generates = 0;
	m_modifierKeyItem.m_lock      = false;
	m_modifierKeyItem.m_client    = 0;
}

CKeyMap::~CKeyMap()
{
	// do nothing
}

void
CKeyMap::addKeyEntry(const KeyItem& item)
{
	// ignore kKeyNone
	if (item.m_id == kKeyNone) {
		return;
	}

	// make a list from the item
	KeyItemList items;
	items.push_back(item);

	// set group and dead key flag on the item
	KeyItem& newItem = items.back();

	// mask the required bits with the sensitive bits
	newItem.m_required &= newItem.m_sensitive;

	// add item list
	entries.push_back(items);
	LOG((CLOG_DEBUG3 "add key: %04x %03x %04x (%04x %04x %04x)s", newItem.m_id, newItem.m_button, newItem.m_client, newItem.m_required, newItem.m_sensitive, newItem.m_generates));
}

void
CKeyMap::addHalfDuplexButton(KeyButton button)
{
	m_halfDuplex.insert(button);
}

void
CKeyMap::clearHalfDuplexModifiers()
{
	m_halfDuplexMods.clear();
}

void
CKeyMap::addHalfDuplexModifier(KeyID key)
{
	m_halfDuplexMods.insert(key);
}

void
CKeyMap::finish()
{
	
}

bool
CKeyMap::isHalfDuplex(KeyID key, KeyButton button) const
{
	return (m_halfDuplex.count(button) + m_halfDuplexMods.count(key) > 0);
}

bool
CKeyMap::isCommand(KeyModifierMask mask) const
{
	return ((mask & getCommandModifiers()) != 0);
}

KeyModifierMask
CKeyMap::getCommandModifiers() const
{
	// we currently treat ctrl, alt, meta and super as command modifiers.
	// some platforms may have a more limited set (OS X only needs Alt)
	// but this works anyway.
	return KeyModifierControl |
			KeyModifierAlt    |
			KeyModifierMeta   |
			KeyModifierSuper;
}

void
CKeyMap::collectButtons(const ModifierToKeys& mods, ButtonToKeyMap& keys)
{
	keys.clear();
	for (ModifierToKeys::const_iterator i = mods.begin();
								i != mods.end(); ++i) {
		keys.insert(std::make_pair(i->second.m_button, &i->second));
	}
}

void
CKeyMap::initModifierKey(KeyItem& item)
{
	item.m_generates = 0;
	item.m_lock      = false;
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
/*
	case kKeyMeta_L:
	case kKeyMeta_R:
		item.m_generates = KeyModifierMeta;
		break;
*/
	case kKeySuper_L:
	case kKeySuper_R:
		item.m_generates = KeyModifierSuper;
		break;
/*
	case kKeyAltGr:
		item.m_generates = KeyModifierAltGr;
		break;
*/
	case kKeyCapsLock:
		item.m_generates = KeyModifierCapsLock;
		item.m_lock      = true;
		break;

	case kKeyNumLock:
		item.m_generates = KeyModifierNumLock;
		item.m_lock      = true;
		break;

	case kKeyScrollLock:
		item.m_generates = KeyModifierScrollLock;
		item.m_lock      = true;
		break;

	default:
		// not a modifier
		break;
	}
}

bool
CKeyMap::keysForKeyItem(const KeyItem& keyItem, 
				ModifierToKeys& activeModifiers,
				KeyModifierMask& currentState, KeyModifierMask desiredState,
				KeyModifierMask overrideModifiers,
				bool isAutoRepeat,
				Keystrokes& keystrokes) const
{
	static const KeyModifierMask s_notRequiredMask =
		KeyModifierAltGr | KeyModifierNumLock | KeyModifierScrollLock;

	EKeystroke type;

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


		// match desiredState as closely as possible.  we must not
		// change any modifiers in keyItem.m_sensitive.  and if the key
		// is a modifier, we don't want to change that modifier.
		LOG((CLOG_DEBUG1 "desired state: %04x %04x,%04x,%04x", desiredState, currentState, keyItem.m_required, keyItem.m_sensitive));
		
		// repeat or press of key
		type = isAutoRepeat ? kKeystrokeRepeat : kKeystrokePress;
	
	addKeystrokes(type, keyItem, activeModifiers, currentState, keystrokes);

	return true;
}

bool
CKeyMap::keysToRestoreModifiers(const KeyItem& keyItem, SInt32,
				ModifierToKeys& activeModifiers,
				KeyModifierMask& currentState,
				const ModifierToKeys& desiredModifiers,
				Keystrokes& keystrokes) const
{
	// XXX -- we're not considering modified modifiers here

	ModifierToKeys oldModifiers = activeModifiers;

	// get the pressed modifier buttons before and after
	ButtonToKeyMap oldKeys, newKeys;
	collectButtons(oldModifiers, oldKeys);
	collectButtons(desiredModifiers, newKeys);

	// release unwanted keys
	for (ModifierToKeys::const_iterator i = oldModifiers.begin();
								i != oldModifiers.end(); ++i) {
		KeyButton button = i->second.m_button;
		if (button != keyItem.m_button && newKeys.count(button) == 0) {
			EKeystroke type = kKeystrokeRelease;
			if (i->second.m_lock) {
				type = kKeystrokeUnmodify;
			}
			addKeystrokes(type, i->second,
								activeModifiers, currentState, keystrokes);
		}
	}

	// press wanted keys
	for (ModifierToKeys::const_iterator i = desiredModifiers.begin();
								i != desiredModifiers.end(); ++i) {
		KeyButton button = i->second.m_button;
		if (button != keyItem.m_button && oldKeys.count(button) == 0) {
			EKeystroke type = kKeystrokePress;
			if (i->second.m_lock) {
				type = kKeystrokeModify;
			}
			addKeystrokes(type, i->second,
								activeModifiers, currentState, keystrokes);
		}
	}

	return true;
}

void
CKeyMap::addKeystrokes(EKeystroke type, const KeyItem& keyItem,
				ModifierToKeys& activeModifiers,
				KeyModifierMask& currentState,
				Keystrokes& keystrokes) const
{
	KeyButton button = keyItem.m_button;
	UInt32 data      = keyItem.m_client;
	switch (type) {
	case kKeystrokePress:
		keystrokes.push_back(Keystroke(button, true, false, data));
		if (keyItem.m_generates != 0) {
			if (!keyItem.m_lock || (currentState & keyItem.m_generates) == 0) {
				// add modifier key and activate modifier
				activeModifiers.insert(std::make_pair(
									keyItem.m_generates, keyItem));
				currentState |= keyItem.m_generates;
			}
			else {
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
			std::pair<ModifierToKeys::iterator,
						ModifierToKeys::iterator> range =
				activeModifiers.equal_range(keyItem.m_generates);
			for (ModifierToKeys::iterator i = range.first;
								i != range.second; ++i) {
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
		keystrokes.push_back(Keystroke(button,  true, true, data));
		// no modifier changes on key repeat
		break;
		
	case kKeystrokeClick:
		keystrokes.push_back(Keystroke(button,  true, false, data));
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
					keystrokes.push_back(Keystroke(button,  true, false, data));
				}
				else {
					// turn half-duplex toggle off (release)
					keystrokes.push_back(Keystroke(button, false, false, data));
				}
			}
			else {
				// toggle (click)
				keystrokes.push_back(Keystroke(button,  true, false, data));
				keystrokes.push_back(Keystroke(button, false, false, data));
			}
		}
		else if (type == kKeystrokeModify) {
			// press modifier
			keystrokes.push_back(Keystroke(button, true, false, data));
		}
		else {
			// release all the keys that generate the modifier that are
			// currently down
			std::pair<ModifierToKeys::const_iterator,
						ModifierToKeys::const_iterator> range =
				activeModifiers.equal_range(keyItem.m_generates);
			for (ModifierToKeys::const_iterator i = range.first;
								i != range.second; ++i) {
				keystrokes.push_back(Keystroke(i->second.m_button,
								false, false, i->second.m_client));
			}
		}

		if (type == kKeystrokeModify) {
			activeModifiers.insert(std::make_pair(
								keyItem.m_generates, keyItem));
			currentState |= keyItem.m_generates;
		}
		else {
			activeModifiers.erase(keyItem.m_generates);
			currentState &= ~keyItem.m_generates;
		}
		break;
	}
}

SInt32
CKeyMap::getNumModifiers(KeyModifierMask state)
{
	SInt32 n = 0;
	for (; state != 0; state >>= 1) {
		if ((state & 1) != 0) {
			++n;
		}
	}
	return n;
}

CString
CKeyMap::formatKey(KeyID key, KeyModifierMask mask)
{
	// initialize tables
	initKeyNameMaps();

	CString x;
	for (SInt32 i = 0; i < kKeyModifierNumBits; ++i) {
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
		}
		else {
			x += CStringUtil::print("\\u%04x", key);
		}
	}
	else if (!x.empty()) {
		// remove trailing '+'
		x.erase(x.size() - 1);
	}
	return x;
}

bool
CKeyMap::parseKey(const CString& x, KeyID& key)
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
		if (!isgraph((unsigned char)x[0])) {
			// unknown key
			return false;
		}
		key = (KeyID)x[0];
	}
	else if (x.size() == 6 && x[0] == '\\' && x[1] == 'u') {
		// escaped unicode (\uXXXX where XXXX is a hex number)
		char* end;
		key = (KeyID)strtol(x.c_str() + 2, &end, 16);
		if (*end != '\0') {
			return false;
		}
	}
	else if (!x.empty()) {
		// unknown key
		return false;
	}

	return true;
}

bool
CKeyMap::parseModifiers(CString& x, KeyModifierMask& mask)
{
	// initialize tables
	initKeyNameMaps();

	mask = 0;
	CString::size_type tb = x.find_first_not_of(" \t", 0);
	while (tb != CString::npos) {
		// get next component
		CString::size_type te = x.find_first_of(" \t+)", tb);
		if (te == CString::npos) {
			te = x.size();
		}
		CString c = x.substr(tb, te - tb);
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
		}
		else {
			// unknown string
			x.erase(0, tb);
			tb = x.find_first_not_of(" \t");
			te = x.find_last_not_of(" \t");
			if (tb == CString::npos) {
				x = "";
			}
			else {
				x = x.substr(tb, te - tb + 1);
			}
			return true;
		}

		// check for '+' or end of string
		tb = x.find_first_not_of(" \t", te);
		if (tb != CString::npos) {
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

void
CKeyMap::initKeyNameMaps()
{
	// initialize tables
	if (s_nameToKeyMap == NULL) {
		s_nameToKeyMap = new CNameToKeyMap;
		s_keyToNameMap = new CKeyToNameMap;
		for (const KeyNameMapEntry* i = kKeyNameMap; i->m_name != NULL; ++i) {
			(*s_nameToKeyMap)[i->m_name] = i->m_id;
			(*s_keyToNameMap)[i->m_id]   = i->m_name;
		}
	}
	if (s_nameToModifierMap == NULL) {
		s_nameToModifierMap = new CNameToModifierMap;
		s_modifierToNameMap = new CModifierToNameMap;
		for (const KeyModifierNameMapEntry* i = kModifierNameMap;
								i->m_name != NULL; ++i) {
			(*s_nameToModifierMap)[i->m_name] = i->m_mask;
			(*s_modifierToNameMap)[i->m_mask] = i->m_name;
		}
	}
}


//
// CKeyMap::KeyItem
//

bool
CKeyMap::KeyItem::operator==(const KeyItem& x) const
{
	return (m_id        == x.m_id        &&
			m_button    == x.m_button    &&
			m_required  == x.m_required  &&
			m_sensitive == x.m_sensitive &&
			m_generates == x.m_generates &&
			m_lock      == x.m_lock      &&
			m_client    == x.m_client);
}


//
// CKeyMap::Keystroke
//

CKeyMap::Keystroke::Keystroke(KeyButton button,
				bool press, bool repeat, UInt32 data) :
	m_type(kButton)
{
	m_data.m_button.m_button = button;
	m_data.m_button.m_press  = press;
	m_data.m_button.m_repeat = repeat;
	m_data.m_button.m_client = data;
}
