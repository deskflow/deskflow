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

//
// COSXKeyState
//

COSXKeyState::COSXKeyState()
{
	// FIXME
}

COSXKeyState::~COSXKeyState()
{
	// FIXME
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
		m_keyMap.insert(std::make_pair(s_keys[i].m_keyID, s_keys[i].m_button));
	}
}

void
COSXKeyState::doFakeKeyEvent(KeyButton button, bool press, bool)
{
	// let system figure out character for us
	CGPostKeyboardEvent(0, static_cast<CGKeyCode>(button), press);
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
