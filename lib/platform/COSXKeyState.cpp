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

struct CKeyEntry {
public:
	KeyID				m_keyID;
	KeyButton			m_button;
};
static const CKeyEntry	s_keys[] = {
	/* ASCII */
	{ ' ',	49 },
	{ '!',	18 },
	{ '\"',	39 },
	{ '#',	20 },
	{ '$',	21 },
	{ '%',	23 },
	{ '&',	26 },
	{ '\'',	39 },
	{ '(',	25 },
	{ ')',	29 },
	{ '*',	28 },
	{ '+',	24 },
	{ ',',	43 },
	{ '-',	27 },
	{ '.',	47 },
	{ '/',	44 },
	{ '0',	29 },
	{ '1',	18 },
	{ '2',	19 },
	{ '3',	20 },
	{ '4',	21 },
	{ '5',	23 },
	{ '6',	22 },
	{ '7',	26 },
	{ '8',	28 },
	{ '9',	25 },
	{ ':',	41 },
	{ ';',	41 },
	{ '<',	43 },
	{ '=',	24 },
	{ '>',	47 },
	{ '?',	44 },
	{ '@',	19 },
	{ 'A',	0 },
	{ 'B',	11 },
	{ 'C',	8 },
	{ 'D',	2 },
	{ 'E',	14 },
	{ 'F',	3 },
	{ 'G',	5 },
	{ 'H',	4 },
	{ 'I',	34 },
	{ 'J',	38 },
	{ 'K',	40 },
	{ 'L',	37 },
	{ 'M',	46 },
	{ 'N',	45 },
	{ 'O',	31 },
	{ 'P',	35 },
	{ 'Q',	12 },
	{ 'R',	15 },
	{ 'S',	1 },
	{ 'T',	17 },
	{ 'U',	32 },
	{ 'V',	9 },
	{ 'W',	13 },
	{ 'X',	7 },
	{ 'Y',	16 },
	{ 'Z',	6 },
	{ '[',	33 },
	{ '\\',	42 },
	{ ']',	30 },
	{ '^',	22 },
	{ '_',	27 },
	{ '`',	50 },
	{ 'a',	0 },
	{ 'b',	11 },
	{ 'c',	8 },
	{ 'd',	2 },
	{ 'e',	14 },
	{ 'f',	3 },
	{ 'g',	5 },
	{ 'h',	4 },
	{ 'i',	34 },
	{ 'j',	38 },
	{ 'k',	40 },
	{ 'l',	37 },
	{ 'm',	46 },
	{ 'n',	45 },
	{ 'o',	31 },
	{ 'p',	35 },
	{ 'q',	12 },
	{ 'r',	15 },
	{ 's',	1 },
	{ 't',	17 },
	{ 'u',	32 },
	{ 'v',	9 },
	{ 'w',	13 },
	{ 'x',	7 },
	{ 'y',	16 },
	{ 'z',	6 },
	{ '{',	33 },
	{ '|',	42 },
	{ '}',	30 },
	{ '~',	50 },

	/* TTY functions */
	{ kKeyBackSpace,	51 },
	{ kKeyTab,			48 },
	{ kKeyLinefeed,		36 },
//	{ kKeyClear,		0xFFFF },
	{ kKeyReturn,		36 },
	{ kKeyPause,		113 },
	{ kKeyScrollLock,	107 },
//	{ kKeySysReq,		0xFFFF }, /* no mapping on apple */
	{ kKeyEscape,		53 },
	{ kKeyDelete,		117 },

	/* cursor control */
	{ kKeyHome,			115 },
	{ kKeyLeft,			123 },
	{ kKeyUp,			126 },
	{ kKeyRight,		124 },
	{ kKeyDown,			125 },
	{ kKeyPageUp,		116 },
	{ kKeyPageDown,		121 },
	{ kKeyEnd,			119 },
	{ kKeyBegin,		115 },

	/* numeric keypad */
	{ kKeyKP_Space,		49 },
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

	/* Function keys */
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

	/* Modifier keys */
	{ kKeyShift_L,		56 },
	{ kKeyShift_R,		56 },
	{ kKeyControl_L,	59 },
	{ kKeyControl_R,	59 },
	{ kKeyAlt_L,		55 },
	{ kKeyAlt_R,		55 },
	{ kKeyCapsLock,		57 },
	{ kKeyNumLock,		71 },
	{ kKeyMeta_L,		58 },
	{ kKeyMeta_R,		58 },
	{ kKeySuper_L,		58 },
	{ kKeySuper_R,		58 },
	{ kKeyLeftTab,		48 }
};

const KeyID COSXKeyState::s_virtualKey[] =
{
	/* 0x00 */ kKeyNone,		// A
	/* 0x01 */ kKeyNone,		// S
	/* 0x02 */ kKeyNone,		// D
	/* 0x03 */ kKeyNone,		// F
	/* 0x04 */ kKeyNone,		// H
	/* 0x05 */ kKeyNone,		// G
	/* 0x06 */ kKeyNone,		// Z
	/* 0x07 */ kKeyNone,		// X
	/* 0x08 */ kKeyNone,		// C
	/* 0x09 */ kKeyNone,		// V
	/* 0x0a */ kKeyNone,		// ~ on some european keyboards
	/* 0x0b */ kKeyNone,		// B
	/* 0x0c */ kKeyNone,		// Q
	/* 0x0d */ kKeyNone,		// W
	/* 0x0e */ kKeyNone,		// E
	/* 0x0f */ kKeyNone,		// R
	/* 0x10 */ kKeyNone,		// Y
	/* 0x11 */ kKeyNone,		// T
	/* 0x12 */ kKeyNone,		// 1
	/* 0x13 */ kKeyNone,		// 2
	/* 0x14 */ kKeyNone,		// 3
	/* 0x15 */ kKeyNone,		// 4			
	/* 0x16 */ kKeyNone,		// 6		
	/* 0x17 */ kKeyNone,		// 5			
	/* 0x18 */ kKeyNone,		// =			
	/* 0x19 */ kKeyNone,		// 9			
	/* 0x1a */ kKeyNone,		// 7
	/* 0x1b */ kKeyNone,		// -
	/* 0x1c */ kKeyNone,		// 8		
	/* 0x1d */ kKeyNone,		// 0	
	/* 0x1e */ kKeyNone,		// ]		
	/* 0x1f */ kKeyNone,		// O	
	/* 0x20 */ kKeyNone,		// U
	/* 0x21 */ kKeyNone,		// [
	/* 0x22 */ kKeyNone,		// I
	/* 0x23 */ kKeyNone,		// P
	/* 0x24 */ kKeyReturn,		// return
	/* 0x25 */ kKeyNone,		// L
	/* 0x26 */ kKeyNone,		// J
	/* 0x27 */ kKeyNone,		// '
	/* 0x28 */ kKeyNone,		// K
	/* 0x29 */ kKeyNone,		// ;
	/* 0x2a */ kKeyNone,		/* \ */
	/* 0x2b */ kKeyNone,		// ,
	/* 0x2c */ kKeyNone,		// /
	/* 0x2d */ kKeyNone,		// M
	/* 0x2e */ kKeyNone,		// M
	/* 0x2f */ kKeyNone,		// .
	/* 0x30 */ kKeyTab,			// tab
	/* 0x31 */ kKeyKP_Space,	// space
	/* 0x32 */ kKeyNone,		// `
	/* 0x33 */ kKeyBackSpace,	// Backspace
	/* 0x34 */ kKeyNone,		// undefined
	/* 0x35 */ kKeyEscape,		// escape
	/* 0x36 */ kKeyNone,		// undefined
	/* 0x37 */ kKeyAlt_L,		// alt
	/* 0x38 */ kKeyShift_L,		// shift
	/* 0x39 */ kKeyCapsLock,	// capslok
	/* 0x3a */ kKeyMeta_L,		// meta
	/* 0x3b */ kKeyControl_L,	// control
	/* 0x3c */ kKeyNone,		// undefined
	/* 0x3d */ kKeyNone,		// undefined
	/* 0x3e */ kKeyNone,		// undefined
	/* 0x3f */ kKeyNone,		// undefined
	/* 0x40 */ kKeyNone,		// undefined
	/* 0x41 */ kKeyKP_Decimal,	// keypad .
	/* 0x42 */ kKeyNone,		// undefined
	/* 0x43 */ kKeyKP_Multiply,	// keypad *
	/* 0x44 */ kKeyNone,		// undefined
	/* 0x45 */ kKeyKP_Add,		// keypad +
	/* 0x46 */ kKeyNone,		// undefined
	/* 0x47 */ kKeyNumLock,		// numlock
	/* 0x48 */ kKeyNone,		// undefined
	/* 0x49 */ kKeyNone,		// undefined
	/* 0x4a */ kKeyNone,		// undefined
	/* 0x4b */ kKeyKP_Divide,	/* keypad \ */
	/* 0x4c */ kKeyKP_Enter,	// keypad enter
	/* 0x4d */ kKeyNone,		// undefined
	/* 0x4e */ kKeyKP_Subtract,	// keypad -
	/* 0x4f */ kKeyNone,		// undefined
	/* 0x50 */ kKeyNone,		// undefined
	/* 0x51 */ kKeyNone,		// undefined
	/* 0x52 */ kKeyKP_0,		// keypad 0
	/* 0x53 */ kKeyKP_1,		// keypad 1
	/* 0x54 */ kKeyKP_2,		// keypad 2
	/* 0x55 */ kKeyKP_3,		// keypad 3
	/* 0x56 */ kKeyKP_4,		// keypad 4
	/* 0x57 */ kKeyKP_5,		// keypad 5
	/* 0x58 */ kKeyKP_6,		// keypad 6
	/* 0x59 */ kKeyKP_7,		// keypad 7
	/* 0x5a */ kKeyKP_8,		// keypad 8
	/* 0x5b */ kKeyKP_9,		// keypad 9
	/* 0x5c */ kKeyNone,		// undefined
	/* 0x5d */ kKeyNone,		// undefined
	/* 0x5e */ kKeyNone,		// undefined
	/* 0x5f */ kKeyNone,		// undefined
	/* 0x60 */ kKeyF5,			// F5
	/* 0x61 */ kKeyF6,			// F6
	/* 0x62 */ kKeyF7,			// F7
	/* 0x63 */ kKeyF3,			// F3
	/* 0x64 */ kKeyF8,			// F8
	/* 0x65 */ kKeyF9,			// F9
	/* 0x66 */ kKeyNone,		// undefined
	/* 0x67 */ kKeyF11,			// F11
	/* 0x68 */ kKeyNone,		// undefined
	/* 0x69 */ kKeyNone,		// undefined
	/* 0x6a */ kKeyNone,		// undefined
	/* 0x6b */ kKeyF10,			// F10
	/* 0x6c */ kKeyNone,		// undefined
	/* 0x6d */ kKeyF12,			// F12
	/* 0x6e */ kKeyNone,		// undefined
	/* 0x6f */ kKeyPause,		// pause
	/* 0x70 */ kKeyNone,		// undefined
	/* 0x71 */ kKeyBegin,		// home
	/* 0x72 */ kKeyPageUp,		// PageUP
	/* 0x73 */ kKeyDelete,		// Delete
	/* 0x74 */ kKeyF4,			// F4
	/* 0x75 */ kKeyEnd,			// end
	/* 0x76 */ kKeyF2,			// F2
	/* 0x77 */ kKeyNone,		// undefined
	/* 0x78 */ kKeyF1,			// F1
	/* 0x79 */ kKeyPageDown,	// PageDown
	/* 0x7a */ kKeyNone,		// undefined
	/* 0x7b */ kKeyLeft,		// left
	/* 0x7c */ kKeyRight,		// right
	/* 0x7d */ kKeyDown,		// down
	/* 0x7e */ kKeyUp,			// up
	
	/* 0x7f */ kKeyNone,		// unassigned
	/* 0x80 */ kKeyNone,		// unassigned
	/* 0x81 */ kKeyNone,		// unassigned
	/* 0x82 */ kKeyNone,		// unassigned
	/* 0x83 */ kKeyNone,		// unassigned
	/* 0x84 */ kKeyNone,		// unassigned
	/* 0x85 */ kKeyNone,		// unassigned
	/* 0x86 */ kKeyNone,		// unassigned
	/* 0x87 */ kKeyNone,		// unassigned
	/* 0x88 */ kKeyNone,		// unassigned
	/* 0x89 */ kKeyNone,		// unassigned
	/* 0x8a */ kKeyNone,		// unassigned
	/* 0x8b */ kKeyNone,		// unassigned
	/* 0x8c */ kKeyNone,		// unassigned
	/* 0x8d */ kKeyNone,		// unassigned
	/* 0x8e */ kKeyNone,		// unassigned
	/* 0x8f */ kKeyNone,		// unassigned
	/* 0x90 */ kKeyNone,		// unassigned
	/* 0x91 */ kKeyNone,		// unassigned
	/* 0x92 */ kKeyNone,		// unassigned
	/* 0x93 */ kKeyNone,		// unassigned
	/* 0x94 */ kKeyNone,		// unassigned
	/* 0x95 */ kKeyNone,		// unassigned
	/* 0x96 */ kKeyNone,		// unassigned
	/* 0x97 */ kKeyNone,		// unassigned
	/* 0x98 */ kKeyNone,		// unassigned
	/* 0x99 */ kKeyNone,		// unassigned
	/* 0x9a */ kKeyNone,		// unassigned
	/* 0x9b */ kKeyNone,		// unassigned
	/* 0x9c */ kKeyNone,		// unassigned
	/* 0x9d */ kKeyNone,		// unassigned
	/* 0x9e */ kKeyNone,		// unassigned
	/* 0x9f */ kKeyNone,		// unassigned
	/* 0xa0 */ kKeyNone,		// unassigned
	/* 0xa1 */ kKeyNone,		// unassigned
	/* 0xa2 */ kKeyNone,		// unassigned
	/* 0xa3 */ kKeyNone,		// unassigned
	/* 0xa4 */ kKeyNone,		// unassigned
	/* 0xa5 */ kKeyNone,		// unassigned
	/* 0xa6 */ kKeyNone,		// unassigned
	/* 0xa7 */ kKeyNone,		// unassigned
	/* 0xa8 */ kKeyNone,		// unassigned
	/* 0xa9 */ kKeyNone,		// unassigned
	/* 0xaa */ kKeyNone,		// unassigned
	/* 0xab */ kKeyNone,		// unassigned
	/* 0xac */ kKeyNone,		// unassigned
	/* 0xad */ kKeyNone,		// unassigned
	/* 0xae */ kKeyNone,		// unassigned
	/* 0xaf */ kKeyNone,		// unassigned
	/* 0xb0 */ kKeyNone,		// unassigned
	/* 0xb1 */ kKeyNone,		// unassigned
	/* 0xb2 */ kKeyNone,		// unassigned
	/* 0xb3 */ kKeyNone,		// unassigned
	/* 0xb4 */ kKeyNone,		// unassigned
	/* 0xb5 */ kKeyNone,		// unassigned
	/* 0xb6 */ kKeyNone,		// unassigned
	/* 0xb7 */ kKeyNone,		// unassigned
	/* 0xb8 */ kKeyNone,		// unassigned
	/* 0xb9 */ kKeyNone,		// unassigned
	/* 0xba */ kKeyNone,		// unassigned
	/* 0xbb */ kKeyNone,		// unassigned
	/* 0xbc */ kKeyNone,		// unassigned
	/* 0xbd */ kKeyNone,		// unassigned
	/* 0xbe */ kKeyNone,		// unassigned
	/* 0xbf */ kKeyNone,		// unassigned
	/* 0xc0 */ kKeyNone,		// unassigned
	/* 0xc1 */ kKeyNone,		// unassigned
	/* 0xc2 */ kKeyNone,		// unassigned
	/* 0xc3 */ kKeyNone,		// unassigned
	/* 0xc4 */ kKeyNone,		// unassigned
	/* 0xc5 */ kKeyNone,		// unassigned
	/* 0xc6 */ kKeyNone,		// unassigned
	/* 0xc7 */ kKeyNone,		// unassigned
	/* 0xc8 */ kKeyNone,		// unassigned
	/* 0xc9 */ kKeyNone,		// unassigned
	/* 0xca */ kKeyNone,		// unassigned
	/* 0xcb */ kKeyNone,		// unassigned
	/* 0xcc */ kKeyNone,		// unassigned
	/* 0xcd */ kKeyNone,		// unassigned
	/* 0xce */ kKeyNone,		// unassigned
	/* 0xcf */ kKeyNone,		// unassigned
	/* 0xd0 */ kKeyNone,		// unassigned
	/* 0xd1 */ kKeyNone,		// unassigned
	/* 0xd2 */ kKeyNone,		// unassigned
	/* 0xd3 */ kKeyNone,		// unassigned
	/* 0xd4 */ kKeyNone,		// unassigned
	/* 0xd5 */ kKeyNone,		// unassigned
	/* 0xd6 */ kKeyNone,		// unassigned
	/* 0xd7 */ kKeyNone,		// unassigned
	/* 0xd8 */ kKeyNone,		// unassigned
	/* 0xd9 */ kKeyNone,		// unassigned
	/* 0xda */ kKeyNone,		// unassigned
	/* 0xdb */ kKeyNone,		// unassigned
	/* 0xdc */ kKeyNone,		// unassigned
	/* 0xdd */ kKeyNone,		// unassigned
	/* 0xde */ kKeyNone,		// unassigned
	/* 0xdf */ kKeyNone,		// unassigned
	/* 0xe0 */ kKeyNone,		// unassigned
	/* 0xe1 */ kKeyNone,		// unassigned
	/* 0xe2 */ kKeyNone,		// unassigned
	/* 0xe3 */ kKeyNone,		// unassigned
	/* 0xe4 */ kKeyNone,		// unassigned
	/* 0xe5 */ kKeyNone,		// unassigned
	/* 0xe6 */ kKeyNone,		// unassigned
	/* 0xe7 */ kKeyNone,		// unassigned
	/* 0xe8 */ kKeyNone,		// unassigned
	/* 0xe9 */ kKeyNone,		// unassigned
	/* 0xea */ kKeyNone,		// unassigned
	/* 0xeb */ kKeyNone,		// unassigned
	/* 0xec */ kKeyNone,		// unassigned
	/* 0xed */ kKeyNone,		// unassigned
	/* 0xee */ kKeyNone,		// unassigned
	/* 0xef */ kKeyNone,		// unassigned
	/* 0xf0 */ kKeyNone,		// unassigned
	/* 0xf1 */ kKeyNone,		// unassigned
	/* 0xf2 */ kKeyNone,		// unassigned
	/* 0xf3 */ kKeyNone,		// unassigned
	/* 0xf4 */ kKeyNone,		// unassigned
	/* 0xf5 */ kKeyNone,		// unassigned
	/* 0xf6 */ kKeyNone,		// unassigned			
	/* 0xf7 */ kKeyNone,		// unassigned			
	/* 0xf8 */ kKeyNone,		// unassigned			
	/* 0xf9 */ kKeyNone,		// unassigned			
	/* 0xfa */ kKeyNone,		// unassigned			
	/* 0xfb */ kKeyNone,		// unassigned			
	/* 0xfc */ kKeyNone,		// unassigned
	/* 0xfd */ kKeyNone,		// unassigned
	/* 0xfe */ kKeyNone,		// unassigned		
	/* 0xff */ kKeyNone			// unassigned
};


//
// COSXKeyState
//

COSXKeyState::COSXKeyState()
{
	// do nothing
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

bool
COSXKeyState::fakeCtrlAltDel()
{
	// pass keys through unchanged
	return false;
}

const char*
COSXKeyState::getKeyName(KeyButton) const
{
	// FIXME
	return "";
}

void
COSXKeyState::doUpdateKeys()
{
	// FIXME -- get the current keyboard state.  call setKeyDown(),
	// setToggled(), and addModifier() as appropriate.

	// save key mapping
	// FIXME -- this probably needs to be more dynamic to support
	// non-english keyboards.  also need to map modifiers needed
	// for each KeyID.
	for (UInt32 i = 0; i < sizeof(s_keys) / sizeof(s_keys[0]); ++i) {
		m_keyMap.insert(std::make_pair(s_keys[i].m_keyID,
							s_keys[i].m_button + 1));
	}
}

void
COSXKeyState::doFakeKeyEvent(KeyButton button, bool press, bool isAutoRepeat)
{
	LOG((CLOG_DEBUG2 "doFakeKeyEvent button:%d, press:%d", button, press));
	// let system figure out character for us
	CGPostKeyboardEvent(0, static_cast<CGKeyCode>(button) - 1, press);
}

KeyButton
COSXKeyState::mapKey(Keystrokes& keys, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const
{
	// look up virtual key
	CKeyMap::const_iterator keyIndex = m_keyMap.find(id);
	if (keyIndex == m_keyMap.end()) {
		return 0;
	}
	CGKeyCode keyCode = keyIndex->second;

	// adjust the modifiers to match the desired modifiers
	Keystrokes undo;
	if (!adjustModifiers(keys, undo, desiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		return 0;
	}

	// add the key event
	Keystroke keystroke;
	keystroke.m_key        = keyCode;
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

	return keyCode;
}

bool
COSXKeyState::adjustModifiers(Keystrokes& /*keys*/,
				Keystrokes& /*undo*/,
				KeyModifierMask /*desiredMask*/) const
{
	// FIXME -- should add necessary modifier events to keys and undo
	return true;
}

KeyID 
COSXKeyState::mapKeyFromEvent(EventRef event, KeyModifierMask* maskOut) const
{
	char c;
	GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,
							NULL, sizeof(c), NULL, &c);

	UInt32 vkCode;
	GetEventParameter(event, kEventParamKeyCode, typeUInt32,
							NULL, sizeof(vkCode), NULL, &vkCode);

	KeyID id = s_virtualKey[vkCode];

	// check if not in table;  map character to key id
	if (id == kKeyNone && c != 0) {
		if ((c & 0x80u) == 0) {
			// ASCII
			id = static_cast<KeyID>(c) & 0xffu;
		}
		else {
			// character is not really ASCII.  instead it's some
			// character in the current ANSI code page.  try to
			// convert that to a Unicode character.  if we fail
			// then use the single byte character as is.
			//FIXME
			id = static_cast<KeyID>(c) & 0xffu;
		}
	}
	
	KeyModifierMask activeMask = getActiveModifiers();
	if (id != kKeyNone && c != 0)  {
		// FIXME
	}
	
	// map modifier key
	if (maskOut != NULL) {
		activeMask &= ~KeyModifierModeSwitch;
		*maskOut = activeMask;
	}

	return id;

}
