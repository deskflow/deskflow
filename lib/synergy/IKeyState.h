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
#include "CString.h"
#include "stdvector.h"

class IKeyState : public IInterface {
public:
	class Keystroke {
	public:
		KeyButton		m_key;
		bool			m_press;
		bool			m_repeat;
	};

	typedef std::vector<Keystroke> Keystrokes;
	typedef std::vector<KeyButton> KeyButtons;

	//! @name manipulators
	//@{

	//! Update the key state
	/*!
	Causes the key state to get updated to reflect the physical keyboard
	state and current keyboard mapping.
	*/
	virtual void		updateKeys() = 0;

	//! Release fake pressed keys
	/*!
	Send fake key events to release keys that aren't physically pressed
	but are logically pressed.
	*/
	virtual void		releaseKeys() = 0;

	//! Mark key as being down
	/*!
	Sets the state of \c key to down or up.
	*/
	virtual void		setKeyDown(KeyButton key, bool down) = 0;

	//! Mark modifier as being toggled on
	/*!
	Sets the state of the keys for the given (single) modifier to be
	toggled on.
	*/
	virtual void		setToggled(KeyModifierMask) = 0;

	//! Add keys for modifier
	/*!
	Sets the keys that are mapped to the given (single) modifier.  For
	example, if buttons 5 and 23 were mapped to KeyModifierShift (perhaps
	as left and right shift keys) then the mask would be KeyModifierShift
	and \c keys would contain 5 and 23.  A modifier with no keys is
	ignored.  All keys must be valid (not zero).  \c keys may be modified
	by the call.
	*/
	virtual void		addModifier(KeyModifierMask, KeyButtons& keys) = 0;

	//! Set toggle key state
	/*!
	Update the local toggle key state to match the given state.
	*/
	virtual void		setToggleState(KeyModifierMask) = 0;

	//@}
	//! @name accessors
	//@{

	//! Test if any key is down
	/*!
	If any key is down then returns one of those keys.  Otherwise returns 0.
	*/
	virtual KeyButton	isAnyKeyDown() const = 0;

	//! Test if key is pressed
	/*!
	Returns true iff the given key is down.  Half-duplex toggles
	should always return false.
	*/
	virtual bool		isKeyDown(KeyButton) const = 0;

	//! Test if modifier is a toggle
	/*!
	Returns true iff the given (single) modifier is a toggle.
	*/
	virtual bool		isToggle(KeyModifierMask) const = 0;

	//! Test if modifier is half-duplex
	/*!
	Returns true iff the given (single) modifier is a half-duplex
	toggle key.
	*/
	virtual bool		isHalfDuplex(KeyModifierMask) const = 0;

	//! Test if modifier is active
	/*!
	Returns true iff the given (single) modifier is currently active.
	*/
	virtual bool		isModifierActive(KeyModifierMask) const = 0;

	//! Get the active modifiers
	/*!
	Returns the modifiers that are currently active.
	*/
	virtual KeyModifierMask
						getActiveModifiers() const = 0;

	//! Get key events to change modifier state
	/*!
	Retrieves the key events necessary to activate (\c desireActive is true)
	or deactivate (\c desireActive is false) the modifier given by \c mask
	by pushing them onto the back of \c keys.  \c mask must specify exactly
	one modifier.  \c undo receives the key events necessary to restore the
	modifier's previous state.  They're pushed onto \c undo in the reverse
	order they should be executed.  Returns true if the modifier can be
	adjusted, false otherwise.
	*/
	virtual bool		mapModifier(Keystrokes& keys, Keystrokes& undo,
							KeyModifierMask mask, bool desireActive) const = 0;

	//! Get modifier mask for key
	/*!
	Returns the modifier mask for \c key.  If \c key is not a modifier
	key then returns 0.
	*/
	virtual KeyModifierMask
						getMaskForKey(KeyButton) const = 0;

	//@}

protected:
	typedef UInt8		KeyState;
	enum EKeyState {
		kDown = 0x01,		//!< Key is down
		kToggled = 0x80		//!< Key is toggled on
	};
};

#endif
