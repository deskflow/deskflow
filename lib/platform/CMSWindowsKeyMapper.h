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
#include "CString.h"
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
	void				updateKey(LPARAM eventLParam);

	//! Set the active keyboard layout
	/*!
	Uses \p keyLayout when finding scan codes via \c keyToScanCode()
	and mapping keys in \c mapKeyFromEvent().
	*/
	void				setKeyLayout(HKL keyLayout);

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
	KeyID				mapKeyFromEvent(WPARAM charAndVirtKey, LPARAM info,
							KeyModifierMask* maskOut, bool* altgr) const;

	//! Check if virtual key is a modifier
	/*!
	Returns true iff \p virtKey refers to a modifier key.
	*/
	bool				isModifier(UINT virtKey) const;

	//! Test shadow key state
	/*!
	Returns true iff the shadow state indicates the key is pressed.
	*/
	bool				isPressed(UINT virtKey) const;

	//! Map button to a virtual key
	/*!
	Returns the virtual key for \c button.
	*/
	UINT				buttonToVirtualKey(KeyButton button) const;

	//! Map virtual key to a button
	/*!
	Returns the button for virtual key \c virtKey.
	*/
	KeyButton			virtualKeyToButton(UINT virtKey) const;

	//! Check for extended key
	/*!
	Returns true iff \c key is an extended key
	*/
	bool				isExtendedKey(KeyButton button) const;

	//! Get current modifier key state
	/*!
	Returns the current modifier key state.
	*/
	KeyModifierMask		getActiveModifiers() const;

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
							const IKeyState& keyState, KeyButton button,
							KeyModifierMask desiredMask,
							KeyModifierMask requiredMask,
							bool isAutoRepeat) const;

	// get keystrokes to get modifiers in a desired state
	bool				adjustModifiers(IKeyState::Keystrokes& keys,
							IKeyState::Keystrokes& undo,
							const IKeyState& keyState,
							KeyModifierMask desiredMask,
							KeyModifierMask requiredMask) const;

	//! Test shadow key toggle state
	/*!
	Returns true iff the shadow state indicates the key is toggled on.
	*/
	bool				isToggled(UINT virtKey) const;

	//! Get shadow modifier key state
	/*!
	Returns the shadow modifier key state.
	*/
	KeyModifierMask		getShadowModifiers(bool needAltGr) const;

	// pass character to ToAsciiEx(), returning what it returns
	int					toAscii(TCHAR c, HKL hkl, bool menu, WORD* chars) const;

	// return true iff \c c is a dead character
	bool				isDeadChar(TCHAR c, HKL hkl, bool menu) const;

private:
	class CModifierKeys {
	public:
		enum { s_maxKeys = 2 };
		KeyModifierMask	m_mask;
		KeyButton		m_keys[s_maxKeys];
	};

	// map of key state for each scan code.  this would be 8 bits
	// except windows reuses some scan codes for "extended" keys
	// we actually need 9 bits.  an example is the left and right
	// alt keys;  they share the same scan code but the right key
	// is "extended".
	BYTE				m_keys[512];
	UINT				m_scanCodeToVirtKey[512];
	KeyButton			m_virtKeyToScanCode[256];
	mutable TCHAR		m_deadKey;
	HKL					m_keyLayout;
	CString				m_keyName;

	static const CModifierKeys	s_modifiers[];
	static const char*	s_vkToName[];
	static const KeyID	s_virtualKey[][2];
	static const UINT	s_mapE000[];
	static const UINT	s_mapEE00[];
	static const UINT	s_mapEF00[];
};

#endif
