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

#ifndef ISECONDARYSCREEN_H
#define ISECONDARYSCREEN_H

#include "IInterface.h"
#include "IKeyState.h"
#include "MouseTypes.h"

//! Secondary screen interface
/*!
This interface defines the methods common to all platform dependent
secondary screen implementations.
*/
class ISecondaryScreen : public IInterface {
public:
	//! @name accessors
	//@{

	//! Fake key press/release
	/*!
	Synthesize a press or release of key \c id.
	*/
	virtual void		fakeKeyEvent(KeyButton id, bool press) const = 0;

	//! Fake ctrl+alt+del
	/*!
	Synthesize a press of ctrl+alt+del.  Return true if processing is
	complete and false if normal key processing should continue.
	*/
	virtual bool		fakeCtrlAltDel() const = 0;

	//! Fake mouse press/release
	/*!
	Synthesize a press or release of mouse button \c id.
	*/
	virtual void		fakeMouseButton(ButtonID id, bool press) const = 0;

	//! Fake mouse move
	/*!
	Synthesize a mouse move to the absolute coordinates \c x,y.
	*/
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const = 0;

	//! Fake mouse wheel
	/*!
	Synthesize a mouse wheel event of amount \c delta.
	*/
	virtual void		fakeMouseWheel(SInt32 delta) const = 0;

	//! Map key press/repeat to keystrokes
	/*!
	Convert a press/repeat of key \c id with the modifiers as given
	in \c desiredMask into the keystrokes necessary to synthesize
	that key event.  This may expand into multiple keys due to modifiers
	that don't match the current modifier state from \c keyState, or to
	needing to compose a character using dead key, or to other reasons.
	Return the platform specific code of the key being pressed.  If \c id
	cannot be mapped or if \c isAutoRepeat is true and the key does not
	auto-repeat then return 0.
	*/
	virtual KeyButton	mapKey(IKeyState::Keystrokes&,
							const IKeyState& keyState, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const = 0;

	//@}
};

#endif
