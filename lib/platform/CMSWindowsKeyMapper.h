/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#ifndef CMSWINDOWSKEYMAPPER_H
#define CMSWINDOWSKEYMAPPER_H

#include "IKeyState.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//! Microsoft Windows key mapper
/*!
This class maps KeyIDs to keystrokes.
*/
class CMSWindowsKeyMapper {
public:
	CMSWindowsKeyMapper();
	~CMSWindowsKeyMapper();

	//! @name manipulators
	//@{

	//! Update key mapper
	/*!
	Updates the key mapper's internal tables according to the
	current keyboard mapping and updates \c keyState.
	*/
	void				update(IKeyState* keyState);

	//! Update shadow key state
	/*!
	Updates the shadow keyboard state.
	*/
	void				updateKey(KeyButton key, bool pressed);

	//@}
	//! @name accessors
	//@{

	//! Map key press/repeat to keystrokes
	/*!
	Converts a press/repeat of key \c id with the modifiers as given
	in \c desiredMask into the keystrokes necessary to synthesize
	that key event.  Returns the platform specific code of the key
	being pressed, or 0 if the key cannot be mapped or \c isAutoRepeat
	is true and the key does not auto-repeat.
	*/
	KeyButton			mapKey(IKeyState::Keystrokes&,
							const IKeyState& keyState, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const;

	//! Map key event to a key
	/*!
	Converts a key event into a KeyID and the shadow modifier state
	to a modifier mask.  If \c altgr is non-NULL it's set to true if
	the key requires AltGr and false otherwise.
	*/
	KeyID				mapKeyFromEvent(WPARAM vkCode, LPARAM info,
							KeyModifierMask* maskOut, bool* altgr) const;

	//! Test shadow key state
	/*!
	Returns true iff the shadow state indicates the key is pressed.
	*/
	bool				isPressed(KeyButton key) const;

	//! Map key to a scan code
	/*!
	Returns the scan code for \c key and possibly adjusts \c key.
	*/
	UINT				keyToScanCode(KeyButton* key) const;

	//! Check for extended key
	/*!
	Returns true iff \c key is an extended key
	*/
	bool				isExtendedKey(KeyButton key) const;

	//! Get name of key
	/*!
	Return a string describing the given key.
	*/
	const char*			getKeyName(KeyButton) const;

	//@}

private:
	// convert a language ID to a code page
	UINT				getCodePageFromLangID(LANGID langid) const;

	// map character \c c given keyboard layout \c hkl to the keystrokes
	// to generate it.
	KeyButton			mapCharacter(IKeyState::Keystrokes& keys,
							const IKeyState& keyState, char c, HKL hkl,
							bool isAutoRepeat) const;

	// map \c virtualKey to the keystrokes to generate it, along with
	// keystrokes to update and restore the modifier state.
	KeyButton			mapToKeystrokes(IKeyState::Keystrokes& keys,
							const IKeyState& keyState, KeyButton virtualKey,
							KeyModifierMask desiredMask,
							KeyModifierMask requiredMask,
							bool isAutoRepeat) const;

	// get keystrokes to get modifiers in a desired state
	bool				adjustModifiers(IKeyState::Keystrokes& keys,
							IKeyState::Keystrokes& undo,
							const IKeyState& keyState,
							KeyModifierMask desiredMask,
							KeyModifierMask requiredMask) const;

	// pass character to ToAsciiEx(), returning what it returns
	int					toAscii(TCHAR c, HKL hkl, bool menu, WORD* chars) const;

	// return true iff \c c is a dead character
	bool				isDeadChar(TCHAR c, HKL hkl, bool menu) const;

	// put back dead key into ToAscii() internal buffer.  returns true
	// iff the character was a dead key.
	bool				putBackDeadChar(TCHAR c, HKL hkl, bool menu) const;

	// get the dead key saved in the given keyboard layout, or 0 if none
	TCHAR				getSavedDeadChar(HKL hkl) const;

	// map the given virtual key, scan code, and keyboard state to a
	// character, if possible.  this has the side effect of updating
	// m_deadKey.
	char				mapToCharacter(UINT vkCode, UINT scanCode,
							BYTE* keys, bool press, bool isMenu, HKL hkl) const;

private:
	class CModifierKeys {
	public:
		enum { s_maxKeys = 2 };
		KeyModifierMask	m_mask;
		KeyButton		m_keys[s_maxKeys];
	};

	BYTE				m_keys[256];
	mutable TCHAR		m_deadKey;

	static const CModifierKeys	s_modifiers[];
	static const char*		s_vkToName[];
	static const KeyID		s_virtualKey[][2];
	static const KeyButton	s_mapE000[];
	static const KeyButton	s_mapEE00[];
	static const KeyButton	s_mapEF00[];
};

#endif
