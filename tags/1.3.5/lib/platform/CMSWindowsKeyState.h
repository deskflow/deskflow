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
#include "stdvector.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CEvent;
class CEventQueueTimer;
class CMSWindowsDesks;

//! Microsoft Windows key mapper
/*!
This class maps KeyIDs to keystrokes.
*/
class CMSWindowsKeyState : public CKeyState {
public:
	CMSWindowsKeyState(CMSWindowsDesks* desks, void* eventTarget);
	virtual ~CMSWindowsKeyState();

	//! @name manipulators
	//@{

	//! Handle screen disabling
	/*!
	Called when screen is disabled.  This is needed to deal with platform
	brokenness.
	*/
	void				disable();

	//! Set the active keyboard layout
	/*!
	Uses \p keyLayout when querying the keyboard.
	*/
	void				setKeyLayout(HKL keyLayout);

	//! Test and set autorepeat state
	/*!
	Returns true if the given button is autorepeating and updates internal
	state.
	*/
	bool				testAutoRepeat(bool press, bool isRepeat, KeyButton);

	//! Remember modifier state
	/*!
	Records the current non-toggle modifier state.
	*/
	void				saveModifiers();

	//! Set effective modifier state
	/*!
	Temporarily sets the non-toggle modifier state to those saved by the
	last call to \c saveModifiers if \p enable is \c true.  Restores the
	modifier state to the current modifier state if \p enable is \c false.
	This is for synthesizing keystrokes on the primary screen when the
	cursor is on a secondary screen.  When on a secondary screen we capture
	all non-toggle modifier state, track the state internally and do not
	pass it on.  So if Alt+F1 synthesizes Alt+X we need to synthesize
	not just X but also Alt, despite the fact that our internal modifier
	state indicates Alt is down, because local apps never saw the Alt down
	event.
	*/
	void				useSavedModifiers(bool enable);

	//@}
	//! @name accessors
	//@{

	//! Map a virtual key to a button
	/*!
	Returns the button for the \p virtualKey.
	*/
	KeyButton			virtualKeyToButton(UINT virtualKey) const;

	//! Map key event to a key
	/*!
	Converts a key event into a KeyID and the shadow modifier state
	to a modifier mask.
	*/
	KeyID				mapKeyFromEvent(WPARAM charAndVirtKey,
							LPARAM info, KeyModifierMask* maskOut) const;

	//! Check if keyboard groups have changed
	/*!
	Returns true iff the number or order of the keyboard groups have
	changed since the last call to updateKeys().
	*/
	bool				didGroupsChange() const;

	//! Map key to virtual key
	/*!
	Returns the virtual key for key \p key or 0 if there's no such virtual
	key.
	*/
	UINT				mapKeyToVirtualKey(KeyID key) const;

	//! Map virtual key and button to KeyID
	/*!
	Returns the KeyID for virtual key \p virtualKey and button \p button
	(button should include the extended key bit), or kKeyNone if there is
	no such key.
	*/
	static KeyID		getKeyID(UINT virtualKey, KeyButton button);

	//@}

	// IKeyState overrides
	virtual void		fakeKeyDown(KeyID id, KeyModifierMask mask,
							KeyButton button);
	virtual void		fakeKeyRepeat(KeyID id, KeyModifierMask mask,
							SInt32 count, KeyButton button);
	virtual bool		fakeCtrlAltDel();
	virtual KeyModifierMask
						pollActiveModifiers() const;
	virtual SInt32		pollActiveGroup() const;
	virtual void		pollPressedKeys(KeyButtonSet& pressedKeys) const;

	// CKeyState overrides
	virtual void		onKey(KeyButton button, bool down,
							KeyModifierMask newState);
	virtual void		sendKeyEvent(void* target,
							bool press, bool isAutoRepeat,
							KeyID key, KeyModifierMask mask,
							SInt32 count, KeyButton button);

protected:
	// CKeyState overrides
	virtual void		getKeyMap(CKeyMap& keyMap);
	virtual void		fakeKey(const Keystroke& keystroke);
	virtual KeyModifierMask&
						getActiveModifiersRValue();

private:
	typedef std::vector<HKL> GroupList;

	// send ctrl+alt+del hotkey event on NT family
	static void			ctrlAltDelThread(void*);

	bool				getGroups(GroupList&) const;
	void				setWindowGroup(SInt32 group);

	void				fixKeys();
	void				handleFixKeys(const CEvent&, void*);

	KeyID				getIDForKey(CKeyMap::KeyItem& item,
							KeyButton button, UINT virtualKey,
							PBYTE keyState, HKL hkl) const;

	void				addKeyEntry(CKeyMap& keyMap, CKeyMap::KeyItem& item);

private:
	// not implemented
	CMSWindowsKeyState(const CMSWindowsKeyState&);
	CMSWindowsKeyState& operator=(const CMSWindowsKeyState&);

private:
	typedef std::map<HKL, SInt32> GroupMap;
	typedef std::map<KeyID, UINT> KeyToVKMap;

	bool				m_is95Family;
	void*				m_eventTarget;
	CMSWindowsDesks*	m_desks;
	HKL					m_keyLayout;
	UINT				m_buttonToVK[512];
	UINT				m_buttonToNumpadVK[512];
	KeyButton			m_virtualKeyToButton[256];
	KeyToVKMap			m_keyToVKMap;

	// the timer used to check for fixing key state
	CEventQueueTimer*	m_fixTimer;

	// the groups (keyboard layouts)
	GroupList			m_groups;
	GroupMap			m_groupMap;

	// the last button that we generated a key down event for.  this
	// is zero if the last key event was a key up.  we use this to
	// synthesize key repeats since the low level keyboard hook can't
	// tell us if an event is a key repeat.
	KeyButton			m_lastDown;

	// modifier tracking
	bool				m_useSavedModifiers;
	KeyModifierMask		m_savedModifiers;
	KeyModifierMask		m_originalSavedModifiers;

	// pointer to ToUnicodeEx.  on win95 family this will be NULL.
	typedef int (WINAPI *ToUnicodeEx_t)(UINT wVirtKey,
										UINT wScanCode,
										PBYTE lpKeyState,
										LPWSTR pwszBuff,
										int cchBuff,
										UINT wFlags,
										HKL dwhkl);
	ToUnicodeEx_t		m_ToUnicodeEx;

	static const KeyID	s_virtualKey[];
};

#endif
