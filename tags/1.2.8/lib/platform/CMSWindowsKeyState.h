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

private:
	typedef std::vector<HKL> GroupList;

	// send ctrl+alt+del hotkey event on NT family
	static void			ctrlAltDelThread(void*);

	bool				getGroups(GroupList&) const;
	void				setWindowGroup(SInt32 group);

	void				fixKeys();
	void				handleFixKeys(const CEvent&, void*);

	KeyID				getKeyID(UINT virtualKey, KeyButton button) const;

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

	// whether any key require AltGr.  if so we reserve the right alt
	// key to be AltGr.
	bool				m_anyAltGr;

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
