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

#ifndef CKEYSTATE_H
#define CKEYSTATE_H

#include "IKeyState.h"
#include "stdvector.h"

//! Core key state
/*!
This class provides key state services.  Subclasses must implement a few
platform specific methods.
*/
class CKeyState : public IKeyState {
public:
	CKeyState();
	virtual ~CKeyState();

	//! @name manipulators
	//@{

	//! Mark key as being down
	/*!
	Sets the state of \p button to down or up.
	*/
	void				setKeyDown(KeyButton button, bool down);

	//! Mark modifier as being toggled on
	/*!
	Sets the state of the keys for the given (single) \p modifier to be
	toggled on.
	*/
	void				setToggled(KeyModifierMask modifier);

	//! Post a key event
	/*!
	Posts a key event.  This may adjust the event or post additional
	events in some circumstances.  If this is overridden it must forward
	to the superclass.
	*/
	virtual void		sendKeyEvent(void* target,
							bool press, bool isAutoRepeat,
							KeyID key, KeyModifierMask mask,
							SInt32 count, KeyButton button);

	//@}
	//! @name accessors
	//@{

	//@}

	// IKeyState overrides
	virtual void		updateKeys();
	virtual void		setHalfDuplexMask(KeyModifierMask);
	virtual void		fakeKeyDown(KeyID id, KeyModifierMask mask,
							KeyButton button);
	virtual void		fakeKeyRepeat(KeyID id, KeyModifierMask mask,
							SInt32 count, KeyButton button);
	virtual void		fakeKeyUp(KeyButton button);
	virtual void		fakeToggle(KeyModifierMask modifier);
	virtual bool		fakeCtrlAltDel() = 0;
	virtual bool		isKeyDown(KeyButton) const;
	virtual bool		isServerKeyDown(KeyButton id) const;
	virtual KeyModifierMask
						getActiveModifiers() const;
	virtual KeyModifierMask
						pollActiveModifiers() const = 0;
	virtual const char*	getKeyName(KeyButton) const = 0;

protected:
	class Keystroke {
	public:
		KeyButton		m_key;
		bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;
	typedef std::vector<KeyButton> KeyButtons;

	//! @name protocted manipulators
	//@{

	//! Add keys for modifier
	/*!
	Sets the buttons that are mapped to the given (single) \p modifier.  For
	example, if buttons 5 and 23 were mapped to KeyModifierShift (perhaps
	as left and right shift keys) then the mask would be KeyModifierShift
	and \c buttons would contain 5 and 23.  A modifier with no keys is
	ignored.  Buttons that are zero are ignored.
	*/
	void				addModifier(KeyModifierMask modifier,
							const KeyButtons& buttons);

	//! Get key events to change modifier state
	/*!
	Retrieves the key events necessary to activate (\p desireActive is true)
	or deactivate (\p desireActive is false) the modifier given by \p mask
	by pushing them onto the back of \p keys.  \p mask must specify exactly
	one modifier.  \p undo receives the key events necessary to restore the
	modifier's previous state.  They're pushed onto \p undo in the reverse
	order they should be executed.  If \p force is false then \p keys and
	\p undo are only changed if the modifier is not currently in the
	desired state.  If \p force is true then \p keys and \p undo are always
	changed.  Returns true if the modifier can be adjusted, false otherwise.
	*/
	bool				mapModifier(Keystrokes& keys, Keystrokes& undo,
							KeyModifierMask mask, bool desireActive,
							bool force = false) const;

	//! Update the key state
	/*!
	Update the key state to reflect the physical keyboard state and
	current keyboard mapping.  This must call \c setKeyDown, \c setToggled,
	and \c addModifier to set the current state.
	*/
	virtual void		doUpdateKeys() = 0;

	//! Fake a key event
	/*!
	Synthesize a key event for \p button.  If \p press is true then
	synthesize a key press and, if false, a key release.  If
	\p isAutoRepeat is true then the event is an auto-repeat.
	*/
	virtual void		doFakeKeyEvent(KeyButton button,
							bool press, bool isAutoRepeat) = 0;

	//! Map key press/repeat to keystrokes
	/*!
	Converts a press/repeat of key \p id with the modifiers as given
	in \p desiredMask into the keystrokes necessary to synthesize
	that key event.  Returns the platform specific code of the key
	being pressed, or 0 if the key cannot be mapped or \p isAutoRepeat
	is true and the key does not auto-repeat.
	*/
	virtual KeyButton	mapKey(Keystrokes& keys, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const = 0;

	//@}

private:
	bool				isHalfDuplex(KeyModifierMask) const;
	bool				isToggle(KeyModifierMask) const;
	bool				isModifierActive(KeyModifierMask) const;
	UInt32				getIndexForModifier(KeyModifierMask) const;
	void				fakeKeyEvents(const Keystrokes&, UInt32 count);
	void				fakeKeyEvent(KeyButton, bool press, bool isAutoRepeat);
	void				updateKeyState(KeyButton serverID,
							KeyButton localID, bool press, bool fake);

private:
	enum {
		kNumModifiers = 9,
		kButtonMask   = kNumButtons - 1
	};
	typedef UInt8		KeyState;
	enum EKeyState {
		kDown = 0x01,		//!< Key is down
		kToggled = 0x02		//!< Key is toggled on
	};

	// modifiers that are half-duplex
	KeyModifierMask		m_halfDuplex;

	// current modifier state
	KeyModifierMask		m_mask;

	// current keyboard state
	KeyState			m_keys[kNumButtons];

	// map from server button ID to local button ID for pressed keys
	KeyButton			m_serverKeyMap[kNumButtons];

	// map button to the modifier mask it represents
	KeyModifierMask		m_keyToMask[kNumButtons];

	// map modifier to buttons with that modifier
	KeyButtons			m_maskToKeys[kNumModifiers];
};

#endif
