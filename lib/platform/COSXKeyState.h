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

#ifndef COSXKEYSTATE_H
#define COSXKEYSTATE_H

#include "CKeyState.h"
#include "stdmap.h"
#include <Carbon/Carbon.h>

//! OS X key state
/*!
A key state for OS X.
*/
class COSXKeyState : public CKeyState {
public:
	COSXKeyState();
	virtual ~COSXKeyState();

	// IKeyState overrides
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
	bool				adjustModifiers(Keystrokes& keys,
							Keystrokes& undo,
							KeyModifierMask desiredMask) const;

private:
	typedef std::map<KeyID, KeyButton> CKeyMap;

	CKeyMap				m_keyMap;
};

#endif
