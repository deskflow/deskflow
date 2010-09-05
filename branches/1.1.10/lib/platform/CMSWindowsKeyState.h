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

#ifndef CMSWINDOWSKEYSTATE_H
#define CMSWINDOWSKEYSTATE_H

#include "CKeyState.h"
#include "CString.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsDesks;

//! Microsoft Windows key mapper
/*!
This class maps KeyIDs to keystrokes.
*/
class CMSWindowsKeyState : public CKeyState {
public:
	CMSWindowsKeyState(CMSWindowsDesks* desks);
	virtual ~CMSWindowsKeyState();

	//! @name accessors
	//@{

	//! Set the active keyboard layout
	/*!
	Uses \p keyLayout when querying the keyboard.
	*/
	void				setKeyLayout(HKL keyLayout);

	//! Check the named virtual key for release
	/*!
	If \p virtualKey isn't really pressed but we think it is then
	update our state and post a key release event to \p eventTarget.
	*/
	void				fixKey(void* eventTarget, UINT virtualKey);

	//! Map key event to a key
	/*!
	Converts a key event into a KeyID and the shadow modifier state
	to a modifier mask.
	*/
	KeyID				mapKeyFromEvent(WPARAM charAndVirtKey,
							LPARAM info, KeyModifierMask* maskOut) const;

	//! Map a virtual key to a button
	/*!
	Returns the button for the \p virtualKey.
	*/
	KeyButton			virtualKeyToButton(UINT virtualKey) const;

	//@}

	// IKeyState overrides
	virtual void		sendKeyEvent(void* target,
							bool press, bool isAutoRepeat,
							KeyID key, KeyModifierMask mask,
							SInt32 count, KeyButton button);
	virtual bool		fakeCtrlAltDel();
	virtual const char*	getKeyName(KeyButton) const;

protected:
	// IKeyState overrides
	virtual void		doUpdateKeys();
	virtual void		doFakeKeyEvent(KeyButton button,
							bool press, bool isAutoRepeat);
	virtual KeyButton	mapKey(Keystrokes& keys, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const;

private:
	// send ctrl+alt+del hotkey event on NT family
	static void			ctrlAltDelThread(void*);

	// convert a language ID to a code page
	UINT				getCodePageFromLangID(LANGID langid) const;

	// map a virtual key to a button.  this tries to deal with the
	// broken win32 API as best it can.
	KeyButton			mapVirtKeyToButton(UINT virtualKey,
							KeyButton& extended) const;

	// same as above and discard extended
	KeyButton			mapVirtKeyToButton(UINT virtualKey) const;

	// map character \c c given keyboard layout \c hkl to the keystrokes
	// to generate it.
	KeyButton			mapCharacter(Keystrokes& keys,
							char c, HKL hkl, bool isAutoRepeat) const;

	// map \c virtualKey to the keystrokes to generate it, along with
	// keystrokes to update and restore the modifier state.
	KeyButton			mapToKeystrokes(Keystrokes& keys, KeyButton button,
							KeyModifierMask desiredMask,
							KeyModifierMask requiredMask,
							bool isAutoRepeat) const;

	// get keystrokes to get modifiers in a desired state
	bool				adjustModifiers(Keystrokes& keys,
							Keystrokes& undo,
							KeyModifierMask desiredMask,
							KeyModifierMask requiredMask) const;

	// pass character to ToAsciiEx(), returning what it returns
	int					toAscii(TCHAR c, HKL hkl, bool menu, WORD* chars) const;

	// return true iff \c c is a dead character
	bool				isDeadChar(TCHAR c, HKL hkl, bool menu) const;

private:
	bool				m_is95Family;
	CMSWindowsDesks*	m_desks;
	HKL					m_keyLayout;
	CString				m_keyName;
	UINT				m_scanCodeToVirtKey[512];
	UINT				m_scanCodeToVirtKeyNumLock[512];
	KeyButton			m_virtKeyToScanCode[256];

	static const char*	s_vkToName[];
	static const KeyID	s_virtualKey[][2];
	static const UINT	s_mapE000[];
	static const UINT	s_mapEE00[];
	static const UINT	s_mapEF00[];
};

#endif
