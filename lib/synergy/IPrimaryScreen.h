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

#ifndef IPRIMARYSCREEN_H
#define IPRIMARYSCREEN_H

#include "IInterface.h"
#include "IKeyState.h"

//! Primary screen interface
/*!
This interface defines the methods common to all platform dependent
primary screen implementations.
*/
class IPrimaryScreen : public IInterface {
public:
// XXX -- may need an interface for sending events
	//! @name manipulators
	//@{

	//! Update configuration
	/*!
	This is called when the configuration has changed.  \c activeSides
	is a bitmask of EDirectionMask indicating which sides of the
	primary screen are linked to clients.  Override to handle the
	possible change in jump zones.
	*/
	virtual void		reconfigure(UInt32 activeSides) = 0;

	//! Warp cursor
	/*!
	Warp the cursor to the absolute coordinates \c x,y.  Also
	discard input events up to and including the warp before
	returning.
	*/
	virtual void		warpCursor(SInt32 x, SInt32 y) = 0;

	//! Install a one-shot timer
	/*!
	Installs a one-shot timer for \c timeout seconds and returns the
	id of the timer.
	*/
// XXX -- need to specify the receiver of the event.  or we should
// pass a job.  need a method to remove the timer?
	virtual UInt32		addOneShotTimer(double timeout) = 0;

	//@}
	//! @name accessors
	//@{

	//! Get jump zone size
	/*!
	Return the jump zone size, the size of the regions on the edges of
	the screen that cause the cursor to jump to another screen.
	*/
	virtual SInt32		getJumpZoneSize() const = 0;

	//! Test if mouse is pressed
	/*!
	Return true if any mouse button is currently pressed.
	*/
	virtual bool		isAnyMouseButtonDown() const = 0;

	//! Get name of key
	/*!
	Return a string describing the given key.
	*/
	virtual const char*	getKeyName(KeyButton) const = 0;

	//@}
};

#endif
