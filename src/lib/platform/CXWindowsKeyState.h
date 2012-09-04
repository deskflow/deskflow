/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CXWINDOWSKEYSTATE_H
#define CXWINDOWSKEYSTATE_H

#include "CKeyState.h"
#include "stdmap.h"
#include "stdvector.h"
#if X_DISPLAY_MISSING
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#	if HAVE_X11_EXTENSIONS_XTEST_H
#		include <X11/extensions/XTest.h>
#	else
#		error The XTest extension is required to build synergy
#	endif
#	if HAVE_XKB_EXTENSION
#		include <X11/extensions/XKBstr.h>
#	endif
#endif

//! X Windows key state
/*!
A key state for X Windows.
*/
class CXWindowsKeyState : public CKeyState {
public:
	typedef std::vector<int> CKeycodeList;
	enum {
		kGroupPoll       = -1,
		kGroupPollAndSet = -2
	};

	CXWindowsKeyState(Display*, bool useXKB);
	CXWindowsKeyState(Display*, bool useXKB,
		IEventQueue& eventQueue, CKeyMap& keyMap);
	~CXWindowsKeyState();

	//! @name modifiers
	//@{

	//! Set active group
	/*!
	Sets the active group to \p group.  This is the group returned by
	\c pollActiveGroup().  If \p group is \c kGroupPoll then
	\c pollActiveGroup() will really poll, but that's a slow operation
	on X11.  If \p group is \c kGroupPollAndSet then this will poll the
	active group now and use it for future calls to \c pollActiveGroup().
	*/
	void				setActiveGroup(SInt32 group);

	//! Set the auto-repeat state
	/*!
	Sets the auto-repeat state.
	*/
	void				setAutoRepeat(const XKeyboardState&);

	//@}
	//! @name accessors
	//@{

	//! Convert X modifier mask to synergy mask
	/*!
	Returns the synergy modifier mask corresponding to the X modifier
	mask in \p state.
	*/
	KeyModifierMask		mapModifiersFromX(unsigned int state) const;

	//! Convert synergy modifier mask to X mask
	/*!
	Converts the synergy modifier mask to the corresponding X modifier
	mask.  Returns \c true if successful and \c false if any modifier
	could not be converted.
	*/
	bool				mapModifiersToX(KeyModifierMask, unsigned int&) const;

	//! Convert synergy key to all corresponding X keycodes
	/*!
	Converts the synergy key \p key to all of the keycodes that map to
	that key.
	*/
	void				mapKeyToKeycodes(KeyID key,
							CKeycodeList& keycodes) const;

	//@}

	// IKeyState overrides
	virtual bool		fakeCtrlAltDel();
	virtual KeyModifierMask
						pollActiveModifiers() const;
	virtual SInt32		pollActiveGroup() const;
	virtual void		pollPressedKeys(KeyButtonSet& pressedKeys) const;

protected:
	// CKeyState overrides
	virtual void		getKeyMap(CKeyMap& keyMap);
	virtual void		fakeKey(const Keystroke& keystroke);

private:
	void				init(Display* display, bool useXKB);
	void				updateKeysymMap(CKeyMap&);
	void				updateKeysymMapXKB(CKeyMap&);
	bool				hasModifiersXKB() const;
	int					getEffectiveGroup(KeyCode, int group) const;
	UInt32				getGroupFromState(unsigned int state) const;

	static void			remapKeyModifiers(KeyID, SInt32,
							CKeyMap::KeyItem&, void*);

private:
	struct XKBModifierInfo {
	public:
		unsigned char	m_level;
		UInt32			m_mask;
		bool			m_lock;
	};

	typedef std::vector<KeyModifierMask> KeyModifierMaskList;
	typedef std::map<KeyModifierMask, unsigned int> KeyModifierToXMask;
	typedef std::multimap<KeyID, KeyCode> KeyToKeyCodeMap;
	typedef std::map<KeyCode, unsigned int> NonXKBModifierMap;
	typedef std::map<UInt32, XKBModifierInfo> XKBModifierMap;

	Display*			m_display;
#if HAVE_XKB_EXTENSION
	XkbDescPtr			m_xkb;
#endif
	SInt32				m_group;
	XKBModifierMap		m_lastGoodXKBModifiers;
	NonXKBModifierMap	m_lastGoodNonXKBModifiers;

	// X modifier (bit number) to synergy modifier (mask) mapping
	KeyModifierMaskList	m_modifierFromX;

	// synergy modifier (mask) to X modifier (mask)
	KeyModifierToXMask	m_modifierToX;

	// map KeyID to all keycodes that can synthesize that KeyID
	KeyToKeyCodeMap		m_keyCodeFromKey;

	// autorepeat state
	XKeyboardState		m_keyboardState;
};

#endif
