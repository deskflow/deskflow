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

#ifndef IKEYSTATE_H
#define IKEYSTATE_H

#include "IInterface.h"
#include "KeyTypes.h"
#include "CEvent.h"

//! Key state interface
/*!
This interface provides access to set and query the keyboard state and
to synthesize key events.
*/
class IKeyState : public IInterface {
public:
	enum {
		kNumButtons = 0x200
	};

	//! Key event data
	class CKeyInfo {
	public:
		static CKeyInfo* alloc(KeyID, KeyModifierMask, KeyButton, SInt32 count);

	public:
		KeyID			m_key;
		KeyModifierMask	m_mask;
		KeyButton		m_button;
		SInt32			m_count;
	};

	//! @name manipulators
	//@{

	//! Update the key state
	/*!
	Causes the key state to get updated to reflect the physical keyboard
	state and current keyboard mapping.
	*/
	virtual void		updateKeys() = 0;

	//! Set half-duplex mask
	/*!
	Sets which modifier toggle keys are half-duplex.  A half-duplex
	toggle key doesn't report a key release when toggled on and
	doesn't report a key press when toggled off.
	*/
	virtual void		setHalfDuplexMask(KeyModifierMask) = 0;

	//! Fake a key press
	/*!
	Synthesizes a key press event and updates the key state.
	*/
	virtual void		fakeKeyDown(KeyID id, KeyModifierMask mask,
							KeyButton button) = 0;

	//! Fake a key repeat
	/*!
	Synthesizes a key repeat event and updates the key state.
	*/
	virtual void		fakeKeyRepeat(KeyID id, KeyModifierMask mask,
							SInt32 count, KeyButton button) = 0;

	//! Fake a key release
	/*!
	Synthesizes a key release event and updates the key state.
	*/
	virtual void		fakeKeyUp(KeyButton button) = 0;

	//! Fake a modifier toggle
	/*!
	Synthesizes key press/release events to toggle the given \p modifier
	and updates the key state.
	*/
	virtual void		fakeToggle(KeyModifierMask modifier) = 0;

	//! Fake ctrl+alt+del
	/*!
	Synthesize a press of ctrl+alt+del.  Return true if processing is
	complete and false if normal key processing should continue.
	*/
	virtual bool		fakeCtrlAltDel() = 0;

	//@}
	//! @name accessors
	//@{

	//! Test if key is pressed
	/*!
	Returns true iff the given key is down.  Half-duplex toggles
	always return false.
	*/
	virtual bool		isKeyDown(KeyButton) const = 0;

	//! Get the active modifiers
	/*!
	Returns the modifiers that are currently active according to our
	shadowed state.
	*/
	virtual KeyModifierMask
						getActiveModifiers() const = 0;

	//! Get the active modifiers from OS
	/*!
	Returns the modifiers that are currently active according to the
	operating system.
	*/
	virtual KeyModifierMask
						pollActiveModifiers() const = 0;

	//! Get name of key
	/*!
	Return a string describing the given key.
	*/
	virtual const char*	getKeyName(KeyButton) const = 0;

	//! Get key down event type.  Event data is CKeyInfo*, count == 1.
	static CEvent::Type	getKeyDownEvent();
	//! Get key up event type.  Event data is CKeyInfo*, count == 1.
	static CEvent::Type	getKeyUpEvent();
	//! Get key repeat event type.  Event data is CKeyInfo*.
	static CEvent::Type	getKeyRepeatEvent();

	//@}

private:
	static CEvent::Type	s_keyDownEvent;
	static CEvent::Type	s_keyUpEvent;
	static CEvent::Type	s_keyRepeatEvent;
};

#endif
