/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#include "COSXKeyState.h"
#include "CLog.h"
#include <stdio.h>

struct CKCHRDeadKeyRecord {
public:
    UInt8               m_tableIndex;
    UInt8               m_virtualKey;
    SInt16              m_numCompletions;
    UInt8               m_completion[1][2];
};

struct CKCHRDeadKeys {
public:
    SInt16              m_numRecords;
    CKCHRDeadKeyRecord  m_records[1];
};

struct CKeyEntry {
public:
	KeyID				m_keyID;
	UInt32				m_virtualKey;
};
// Hardcoded virtual key table.  Oddly, Apple doesn't document the
// meaning of virtual key codes.  The whole point of *virtual* key
// codes is to make them hardware independent so these codes should
// be constant across OS versions and hardware.  Yet they don't
// tell us what codes map to what keys so we have to figure it out
// for ourselves.
//
// Note that some virtual keys codes appear more than once.  The
// first instance of a virtual key code maps to the KeyID that we
// want to generate for that code.  The others are for mapping
// different KeyIDs to a single key code.
static const CKeyEntry	s_controlKeys[] = {
	// TTY functions
	{ kKeyBackSpace,	51 },
	{ kKeyTab,			48 },
	{ kKeyLeftTab,		48 },
	{ kKeyReturn,		36 },
	{ kKeyLinefeed,		36 },
//	{ kKeyClear,		0xFFFF }, /* no mapping on apple */
//	{ kKeyPause,		0xFFFF }, /* no mapping on apple */
//	{ kKeyScrollLock,	0xFFFF }, /* no mapping on apple */
//	{ kKeySysReq,		0xFFFF }, /* no mapping on apple */
	{ kKeyEscape,		53 },
	{ kKeyDelete,		117 },

	// cursor control
	{ kKeyHome,			115 },
	{ kKeyBegin,		115 },
	{ kKeyLeft,			123 },
	{ kKeyUp,			126 },
	{ kKeyRight,		124 },
	{ kKeyDown,			125 },
	{ kKeyPageUp,		116 },
	{ kKeyPageDown,		121 },
	{ kKeyEnd,			119 },

	// numeric keypad
	{ kKeyKP_0,			82 },
	{ kKeyKP_1,			83 },
	{ kKeyKP_2,			84 },
	{ kKeyKP_3,			85 },
	{ kKeyKP_4,			86 },
	{ kKeyKP_5,			87 },
	{ kKeyKP_6,			88 },
	{ kKeyKP_7,			89 },
	{ kKeyKP_8,			91 },
	{ kKeyKP_9,			92 },
	{ kKeyKP_Enter,		76 },
	{ kKeyKP_Decimal,	65 },
	{ kKeyKP_Add,		69 },
	{ kKeyKP_Subtract,	78 },
	{ kKeyKP_Multiply,	67 },
	{ kKeyKP_Divide,	75 },

	// function keys
	{ kKeyF1,			122 },
	{ kKeyF2,			120 },
	{ kKeyF3,			99 },
	{ kKeyF4,			118 },
	{ kKeyF5,			96 },
	{ kKeyF6,			97 },
	{ kKeyF7,			98 },
	{ kKeyF8,			100 },
	{ kKeyF9,			101 },
	{ kKeyF10,			109 },
	{ kKeyF11,			103 },
	{ kKeyF12,			111 },
	{ kKeyF13,			105 },
	{ kKeyF14,			107 },
	{ kKeyF15,			113 },

	// misc keys
	{ kKeyHelp,			114 },

	// modifier keys.  i don't know how to make the mac properly
	// interpret the right hand versions of modifier keys so they're
	// currently mapped to the left hand version.
	{ kKeyShift_L,		56 },
	{ kKeyShift_R,		56 /*60*/ },
	{ kKeyControl_L,	59 },
	{ kKeyControl_R,	59 /*62*/ },
	{ kKeyAlt_L,		55 },
	{ kKeyAlt_R,		55 },
	{ kKeySuper_L,		58 },
	{ kKeySuper_R,		58 /*61*/ },
	{ kKeyMeta_L,		58 },
	{ kKeyMeta_R,		58 /*61*/ },
	{ kKeyCapsLock,		57 },
	{ kKeyNumLock,		71 }
};

//
// COSXKeyState
//

COSXKeyState::COSXKeyState()
{
	setHalfDuplexMask(0);
	SInt16 currentKeyScript = GetScriptManagerVariable(smKeyScript);
	SInt16 keyboardLayoutID = GetScriptVariable(currentKeyScript, smScriptKeys);
	setKeyboardLayout(keyboardLayoutID);
}

COSXKeyState::~COSXKeyState()
{
	// do nothing
}

void
COSXKeyState::sendKeyEvent(void* target,
				bool press, bool isAutoRepeat,
				KeyID key, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	if (press || isAutoRepeat) {
		// send key
		if (press) {
			CKeyState::sendKeyEvent(target, true, false,
							key, mask, 1, button);
			if (count > 0) {
				--count;
			}
		}

		if (count >= 1) {
			CKeyState::sendKeyEvent(target, true, true,
							key, mask, count, button);
		}

	}
	else {
		// do key up
		CKeyState::sendKeyEvent(target, false, false, key, mask, 1, button);
	}
}

void
COSXKeyState::setHalfDuplexMask(KeyModifierMask mask)
{
	CKeyState::setHalfDuplexMask(mask | KeyModifierCapsLock);
}

bool
COSXKeyState::fakeCtrlAltDel()
{
	// pass keys through unchanged
	return false;
}

const char*
COSXKeyState::getKeyName(KeyButton button) const
{
	static char name[10];
	sprintf(name, "vk 0x%02x", button);
	return name;
}

void
COSXKeyState::doUpdateKeys()
{
	// save key mapping
	m_keyMap.clear();
	if (!filluchrKeysMap(m_keyMap, m_capsLockSet)) {
		fillKCHRKeysMap(m_keyMap, m_capsLockSet);
	}
	fillSpecialKeys(m_keyMap, m_virtualKeyMap);

	// add modifiers
	KeyButtons keys;
	addKeyButton(keys, kKeyShift_L);
	addKeyButton(keys, kKeyShift_R);
	addModifier(KeyModifierShift, keys);
	keys.clear();
	addKeyButton(keys, kKeyControl_L);
	addKeyButton(keys, kKeyControl_R);
	addModifier(KeyModifierControl, keys);
	keys.clear();
	addKeyButton(keys, kKeyAlt_L);
	addKeyButton(keys, kKeyAlt_R);
	addModifier(KeyModifierAlt, keys);
	keys.clear();
	addKeyButton(keys, kKeySuper_L);
	addKeyButton(keys, kKeySuper_R);
	addModifier(KeyModifierSuper, keys);
	keys.clear();
	addKeyButton(keys, kKeyCapsLock);
	addModifier(KeyModifierCapsLock, keys);
	keys.clear();
	addKeyButton(keys, kKeyNumLock);
	addModifier(KeyModifierNumLock, keys);
	keys.clear();

	// FIXME -- get the current keyboard state.  call setKeyDown()
	// and setToggled() as appropriate.
}

void
COSXKeyState::doFakeKeyEvent(KeyButton button, bool press, bool)
{
	LOG((CLOG_DEBUG2 "doFakeKeyEvent button:%d, press:%d", button, press));
	// let system figure out character for us
	CGPostKeyboardEvent(0, mapKeyButtonToVirtualKey(button), press);
}

KeyButton
COSXKeyState::mapKey(Keystrokes& keys, KeyID id,
				KeyModifierMask /*desiredMask*/,
				bool isAutoRepeat) const
{
	// look up virtual key
	CKeyIDMap::const_iterator keyIndex = m_keyMap.find(id);
	if (keyIndex == m_keyMap.end()) {
		return 0;
	}
	const CKeySequence& sequence = keyIndex->second;
	if (sequence.empty()) {
		return 0;
	}

	// if the virtual key is caps-lock sensitive then suppress shift
	KeyModifierMask mask = ~0;
	if (m_capsLockSet.count(id) != 0) {
		mask &= ~KeyModifierShift;
	}

	// FIXME -- for both calls to addKeystrokes below we'd prefer to use
	// a required mask that generates the same character but matches
	// the desiredMask as closely as possible.
	// FIXME -- would prefer to not restore the modifier keys after each
	// dead key since it's unnecessary but we don't have a mechanism
	// for tracking the modifier state without actually updating the
	// internal keyboard state.  we'd have to track the state to
	// ensure we adjust the right modifiers for remaining dead keys
	// and the final key.

	// add dead keys
	for (size_t i = 0; i < sequence.size() - 1; ++i) {
		// simulate press
		KeyButton keyButton =
			addKeystrokes(keys, sequence[i].m_button,
							sequence[i].m_requiredState,
							sequence[i].m_requiredMask, false);

		// simulate release
		Keystroke keystroke;
		keystroke.m_key = keyButton;
		keystroke.m_press  = false;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
	}

	// add final key
	return addKeystrokes(keys, sequence.back().m_button,
							sequence.back().m_requiredState,
							sequence.back().m_requiredMask & mask,
							isAutoRepeat);
}

KeyButton
COSXKeyState::addKeystrokes(Keystrokes& keys, KeyButton keyButton,
				KeyModifierMask desiredMask, KeyModifierMask requiredMask,
				bool isAutoRepeat) const
{
	// adjust the modifiers
	Keystrokes undo;
	if (!adjustModifiers(keys, undo, desiredMask, requiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		return 0;
	}

	// add the key event
	Keystroke keystroke;
	keystroke.m_key        = keyButton;
	if (!isAutoRepeat) {
		keystroke.m_press  = true;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
	}
	else {
		keystroke.m_press  = false;
		keystroke.m_repeat = true;
		keys.push_back(keystroke);
		keystroke.m_press  = true;
		keys.push_back(keystroke);
	}

	// put undo keystrokes at end of keystrokes in reverse order
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	return keyButton;
}

bool
COSXKeyState::adjustModifiers(Keystrokes& keys,
				Keystrokes& undo,
				KeyModifierMask desiredMask,
				KeyModifierMask requiredMask) const
{
	// for each modifier in requiredMask make sure the current state
	// of that modifier matches the bit in desiredMask.
	for (KeyModifierMask mask = 1u; requiredMask != 0; mask <<= 1) {
		if ((mask & requiredMask) != 0) {
			bool active = ((desiredMask & mask) != 0);
			if (!mapModifier(keys, undo, mask, active)) {
				return false;
			}
			requiredMask ^= mask;
		}
	}
	return true;
}

KeyButton 
COSXKeyState::mapKeyFromEvent(CKeyIDs& ids,
				KeyModifierMask* maskOut, EventRef event) const
{
	ids.clear();

	// map modifier key
	if (maskOut != NULL) {
		KeyModifierMask activeMask = getActiveModifiers();
		activeMask &= ~KeyModifierModeSwitch;
		*maskOut    = activeMask;
	}

	// get virtual key
	UInt32 vkCode;
	GetEventParameter(event, kEventParamKeyCode, typeUInt32,
							NULL, sizeof(vkCode), NULL, &vkCode);

	// handle up events
	UInt32 eventKind = GetEventKind(event);
	if (eventKind == kEventRawKeyUp) {
		// the id isn't used.  we just need the same button we used on
		// the key press.  note that we don't use or reset the dead key
		// state;  up events should not affect the dead key state.
		ids.push_back(kKeyNone);
		return mapVirtualKeyToKeyButton(vkCode);
	}

	// check for special keys
	CVirtualKeyMap::const_iterator i = m_virtualKeyMap.find(vkCode);
	if (i != m_virtualKeyMap.end()) {
		m_deadKeyState = 0;
		ids.push_back(i->second);
		return mapVirtualKeyToKeyButton(vkCode);
	}

	// check for character keys
	if (m_uchrResource != NULL) {
		// FIXME -- implement this
	}
	else if (m_KCHRResource != NULL) {
		// get the event modifiers and remove the command and control
		// keys.
		UInt32 modifiers;
		GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
							NULL, sizeof(modifiers), NULL, &modifiers);
		modifiers &= ~(cmdKey | controlKey | rightControlKey);

		// build keycode
		UInt16 keycode =
			static_cast<UInt16>((modifiers & 0xff00u) | (vkCode & 0x00ffu));

		// translate key
		UInt32 result = KeyTranslate(m_KCHRResource, keycode, &m_deadKeyState);

		// get the characters
		UInt8 c1 = static_cast<UInt8>((result >> 16) & 0xffu);
		UInt8 c2 = static_cast<UInt8>( result        & 0xffu);
		if (c2 != 0) {
			m_deadKeyState = 0;
			if (c1 != 0) {
				ids.push_back(charToKeyID(c1));
			}
			ids.push_back(charToKeyID(c2));
			return mapVirtualKeyToKeyButton(vkCode);
		}
	}

	return 0;
}

void
COSXKeyState::addKeyButton(KeyButtons& keys, KeyID id) const
{
	CKeyIDMap::const_iterator keyIndex = m_keyMap.find(id);
	if (keyIndex == m_keyMap.end()) {
		return;
	}
	keys.push_back(keyIndex->second[0].m_button);
}

void
COSXKeyState::handleModifierKeys(void* target,
				KeyModifierMask oldMask, KeyModifierMask newMask)
{
	// compute changed modifiers
	KeyModifierMask changed = (oldMask ^ newMask);

	// synthesize changed modifier keys
	if ((changed & KeyModifierShift) != 0) {
		handleModifierKey(target, kKeyShift_L,
							(newMask & KeyModifierShift) != 0);
	}
	if ((changed & KeyModifierControl) != 0) {
		handleModifierKey(target, kKeyControl_L,
							(newMask & KeyModifierControl) != 0);
	}
	if ((changed & KeyModifierAlt) != 0) {
		handleModifierKey(target, kKeyAlt_L,
							(newMask & KeyModifierAlt) != 0);
	}
	if ((changed & KeyModifierSuper) != 0) {
		handleModifierKey(target, kKeySuper_L,
							(newMask & KeyModifierSuper) != 0);
	}
	if ((changed & KeyModifierCapsLock) != 0) {
		handleModifierKey(target, kKeyCapsLock,
							(newMask & KeyModifierCapsLock) != 0);
	}
}

void
COSXKeyState::handleModifierKey(void* target, KeyID id, bool down)
{
	CKeyIDMap::const_iterator keyIndex = m_keyMap.find(id);
	if (keyIndex == m_keyMap.end()) {
		return;
	}
	KeyButton button = keyIndex->second[0].m_button;
	setKeyDown(button, down);
	sendKeyEvent(target, down, false, id, getActiveModifiers(), 0, button);
}

void
COSXKeyState::checkKeyboardLayout()
{
	SInt16 currentKeyScript = GetScriptManagerVariable(smKeyScript);
	SInt16 keyboardLayoutID = GetScriptVariable(currentKeyScript, smScriptKeys);
	if (keyboardLayoutID != m_keyboardLayoutID) {
		// layout changed
		setKeyboardLayout(keyboardLayoutID);
		doUpdateKeys();
	}
}

void
COSXKeyState::setKeyboardLayout(SInt16 keyboardLayoutID)
{
	m_keyboardLayoutID = keyboardLayoutID;
	m_deadKeyState     = 0;
	m_KCHRHandle       = GetResource('KCHR', m_keyboardLayoutID);
	m_uchrHandle       = GetResource('uchr', m_keyboardLayoutID);
	m_KCHRResource     = NULL;
	m_uchrResource     = NULL;
/* FIXME -- don't use uchr resource yet
	if (m_uchrHandle != NULL) {
		m_uchrResource = reinterpret_cast<UCKeyboardLayout*>(*m_uchrHandle);
	}
	else */if (m_KCHRHandle != NULL) {
		m_KCHRResource = reinterpret_cast<CKCHRResource*>(*m_KCHRHandle);
	}
}

void
COSXKeyState::fillSpecialKeys(CKeyIDMap& keyMap,
				CVirtualKeyMap& virtualKeyMap) const
{
	// FIXME -- would like to avoid hard coded tables
	CKeyEventInfo info;
	for (UInt32 i = 0; i < sizeof(s_controlKeys) /
							sizeof(s_controlKeys[0]); ++i) {
		const CKeyEntry& entry = s_controlKeys[i];
		KeyID keyID            = entry.m_keyID;
		info.m_button          = mapVirtualKeyToKeyButton(entry.m_virtualKey);
		info.m_requiredMask    = 0;
		info.m_requiredState   = 0;
		if (keyMap.count(keyID) == 0) {
			keyMap[keyID].push_back(info);
		}
		if (virtualKeyMap.count(entry.m_virtualKey) == 0) {
			virtualKeyMap[entry.m_virtualKey] = entry.m_keyID;
		}
	}
}

bool
COSXKeyState::fillKCHRKeysMap(CKeyIDMap& keyMap, CKeySet& capsLockSet) const
{
	assert(m_KCHRResource != NULL);

	CKCHRResource* r = m_KCHRResource;

	// note caps-lock sensitive keys
	SInt32 uIndex  = r->m_tableSelectionIndex[0];
	SInt32 clIndex = r->m_tableSelectionIndex[alphaLock >> 8];
	for (SInt32 j = 0; j < 128; ++j) {
		UInt8 c = r->m_characterTables[clIndex][j];
		if (r->m_characterTables[uIndex][j] != c) {
			KeyID keyID = charToKeyID(c);
			capsLockSet.insert(keyID);
		}
	}

	// build non-composed keys to virtual keys mapping
	std::map<UInt8, CKeyEventInfo> vkMap;
	for (SInt32 i = 0; i < r->m_numTables; ++i) {
		// determine the modifier keys for table i
		KeyModifierMask mask =
			maskForTable(static_cast<UInt8>(i), r->m_tableSelectionIndex);

		// build the KeyID to virtual key map
		for (SInt32 j = 0; j < 128; ++j) {
			// get character
			UInt8 c = r->m_characterTables[i][j];

			// save key info
			// FIXME -- should set only those bits in m_requiredMask that
			// correspond to modifiers that are truly necessary to
			// generate the character.  this mostly works as-is, though.
			CKeyEventInfo info;
			info.m_button        = mapVirtualKeyToKeyButton(j);
			info.m_requiredMask  = mask;
			info.m_requiredState = mask;

			// save character to virtual key mapping
			if (vkMap.count(c) == 0) {
				vkMap[c] = info;
			}

			// skip non-glyph character
			if (c < 32 || c == 127) {
				continue;
			}

			// map character to KeyID
			KeyID keyID = charToKeyID(c);

			// if we've seen this character already then do nothing
			if (keyMap.count(keyID) != 0) {
				continue;
			}

			// save entry for character
			keyMap[keyID].push_back(info);
		}
	}

	// build composed keys to virtual keys mapping
	CKCHRDeadKeys* dkp =
		reinterpret_cast<CKCHRDeadKeys*>(r->m_characterTables[r->m_numTables]);
	CKCHRDeadKeyRecord* dkr = dkp->m_records;
	for (SInt32 i = 0; i < dkp->m_numRecords; ++i) {
		// determine the modifier keys for table i
		KeyModifierMask mask =
			maskForTable(dkr->m_tableIndex, r->m_tableSelectionIndex);

		// map each completion
		for (SInt32 j = 0; j < dkr->m_numCompletions; ++j) {
			// get character
			UInt8 c = dkr->m_completion[j][1];

			// skip non-glyph character
			if (c < 32 || c == 127) {
				continue;
			}

			// map character to KeyID
			KeyID keyID = charToKeyID(c);

			// if we've seen this character already then do nothing
			if (keyMap.count(keyID) != 0) {
				continue;
			}

			// map keyID, first to the dead key then to uncomposed
			// character.  we must find a virtual key that maps to
			// to the uncomposed character.
			if (vkMap.count(dkr->m_completion[j][0]) != 0) {
				CKeySequence& sequence = keyMap[keyID];

				// save key info
				// FIXME -- should set only those bits in m_requiredMask that
				// correspond to modifiers that are truly necessary to
				// generate the character.  this mostly works as-is, though.
				CKeyEventInfo info;
				info.m_button        = mapVirtualKeyToKeyButton(
											dkr->m_virtualKey);
				info.m_requiredMask  = mask;
				info.m_requiredState = mask;

				sequence.push_back(info);
				sequence.push_back(vkMap[dkr->m_completion[j][0]]);
			}
		}

		// next table.  skip all the completions and the no match
		// pair to get the next table.
		dkr = reinterpret_cast<CKCHRDeadKeyRecord*>(
							dkr->m_completion[dkr->m_numCompletions + 1]);
	}

	return true;
}

bool
COSXKeyState::filluchrKeysMap(CKeyIDMap&, CKeySet&) const
{
	// FIXME -- implement this
	return false;
}

KeyButton
COSXKeyState::mapVirtualKeyToKeyButton(UInt32 keyCode)
{
	// 'A' maps to 0 so shift every id
	return static_cast<KeyButton>(keyCode + KeyButtonOffset);
}

UInt32
COSXKeyState::mapKeyButtonToVirtualKey(KeyButton keyButton)
{
	return static_cast<UInt32>(keyButton - KeyButtonOffset);
}

KeyID
COSXKeyState::charToKeyID(UInt8 c)
{
	if (c == 0) {
		return kKeyNone;
	}
    else if (c >= 32 && c < 127) {
		// ASCII
        return static_cast<KeyID>(c);
    }
    else {
        // create string with character
        char str[2];
        str[0] = static_cast<char>(c);
        str[1] = 0;

        // convert to unicode
        CFStringRef cfString =
            CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                            str, GetScriptManagerVariable(smKeyScript),
                            kCFAllocatorNull);

        // convert to precomposed
        CFMutableStringRef mcfString =
            CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
        CFRelease(cfString);
        CFStringNormalize(mcfString, kCFStringNormalizationFormC);

        // check result
        int unicodeLength = CFStringGetLength(mcfString);
        if (unicodeLength == 0) {
            return kKeyNone;
        }
        if (unicodeLength > 1) {
			// FIXME -- more than one character, we should handle this
            return kKeyNone;
        }

        // get unicode character
        UniChar uc = CFStringGetCharacterAtIndex(mcfString, 0);
        CFRelease(mcfString);

        // convert to KeyID
        return static_cast<KeyID>(uc);
    }
}

KeyID
COSXKeyState::unicharToKeyID(UniChar c)
{
	return static_cast<KeyID>(c);
}

KeyModifierMask
COSXKeyState::maskForTable(UInt8 i, UInt8* tableSelectors)
{
    // this is a table of 0 to 255 sorted by the number of 1 bits then
	// numerical order.
    static const UInt8 s_indexTable[] = {
0, 1, 2, 4, 8, 16, 32, 64, 128, 3, 5, 6, 9, 10, 12, 17,
18, 20, 24, 33, 34, 36, 40, 48, 65, 66, 68, 72, 80, 96, 129, 130,
132, 136, 144, 160, 192, 7, 11, 13, 14, 19, 21, 22, 25, 26, 28, 35,
37, 38, 41, 42, 44, 49, 50, 52, 56, 67, 69, 70, 73, 74, 76, 81,
82, 84, 88, 97, 98, 100, 104, 112, 131, 133, 134, 137, 138, 140, 145, 146,
148, 152, 161, 162, 164, 168, 176, 193, 194, 196, 200, 208, 224, 15, 23, 27,
29, 30, 39, 43, 45, 46, 51, 53, 54, 57, 58, 60, 71, 75, 77, 78,
83, 85, 86, 89, 90, 92, 99, 101, 102, 105, 106, 108, 113, 114, 116, 120,
135, 139, 141, 142, 147, 149, 150, 153, 154, 156, 163, 165, 166, 169, 170, 172,
177, 178, 180, 184, 195, 197, 198, 201, 202, 204, 209, 210, 212, 216, 225, 226,
228, 232, 240, 31, 47, 55, 59, 61, 62, 79, 87, 91, 93, 94, 103, 107,
109, 110, 115, 117, 118, 121, 122, 124, 143, 151, 155, 157, 158, 167, 171, 173,
174, 179, 181, 182, 185, 186, 188, 199, 203, 205, 206, 211, 213, 214, 217, 218,
220, 227, 229, 230, 233, 234, 236, 241, 242, 244, 248, 63, 95, 111, 119, 123,
125, 126, 159, 175, 183, 187, 189, 190, 207, 215, 219, 221, 222, 231, 235, 237,
238, 243, 245, 246, 249, 250, 252, 127, 191, 223, 239, 247, 251, 253, 254, 255
    };

    // find first entry in tableSelectors that maps to i.  this is the
	// one that uses the fewest modifier keys.
    for (UInt32 j = 0; j < 256; ++j) {
        if (tableSelectors[s_indexTable[j]] == i) {
			// convert our mask to a traditional mac modifier mask
			// (which just means shifting it left 8 bits).
			UInt16 macMask = (static_cast<UInt16>(s_indexTable[j]) << 8);

			// convert the mac modifier mask to our mask.
			KeyModifierMask mask = 0;
			if ((macMask & (shiftKey | rightShiftKey)) != 0) {
				mask |= KeyModifierShift;
			}
			if ((macMask & (controlKey | rightControlKey)) != 0) {
				mask |= KeyModifierControl;
			}
			if ((macMask & cmdKey) != 0) {
				mask |= KeyModifierAlt;
			}
			if ((macMask & (optionKey | rightOptionKey)) != 0) {
				mask |= KeyModifierSuper;
			}
			if ((macMask & alphaLock) != 0) {
				mask |= KeyModifierCapsLock;
			}
            return mask;
        }
    }

    // should never get here since we've tried every 8 bit number
    return 0;
}
