/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#ifndef ISCREEN_H
#define ISCREEN_H

#include "IInterface.h"
#include "ClipboardTypes.h"

class IClipboard;

//! Screen interface
/*!
This interface defines the methods common to all platform dependent
screen implementations that are use by both primary and secondary
screens.
*/
class IScreen : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Open screen
	/*!
	Called to open and initialize the screen.  Throw XScreenUnavailable
	if the screen cannot be opened but retrying later may succeed.
	Otherwise throw some other XScreenOpenFailure exception.
	*/
	virtual void		open() = 0;

	//! Run event loop
	/*!
	Run the event loop and return when exitMainLoop() is called.
	This must be called between a successful open() and close().
	*/
	virtual void		mainLoop() = 0;

	//! Exit event loop
	/*!
	Force mainLoop() to return.  This call can return before
	mainLoop() does (i.e. asynchronously).  This may only be
	called between a successful open() and close().
	*/
	virtual void		exitMainLoop() = 0;

	//! Close screen
	/*!
	Called to close the screen.  close() should quietly ignore calls
	that don't have a matching successful call to open().
	*/
	virtual void		close() = 0;

	//! Set clipboard
	/*!
	Set the contents of the system clipboard indicated by \c id.
	*/
	virtual bool		setClipboard(ClipboardID id, const IClipboard*) = 0;

	//! Check clipboard owner
	/*!
	Check ownership of all clipboards and notify an IScreenReceiver (set
	through some other interface) if any changed.  This is used as a
	backup in case the system doesn't reliably report clipboard ownership
	changes.
	*/
	virtual void		checkClipboards() = 0;

	//! Open screen saver
	/*!
	Open the screen saver.  If \c notify is true then this object must
	call an IScreenEventHandler's (set through some other interface)
	onScreenSaver() when the screensaver activates or deactivates until
	it's closed.  If \c notify is false then the screen saver is
	disabled on open and restored on close.
	*/
	virtual void		openScreensaver(bool notify) = 0;

	//! Close screen saver
	/*!
	// Close the screen saver.  Stop reporting screen saver activation
	and deactivation and, if the screen saver was disabled by
	openScreensaver(), enable the screen saver.
	*/
	virtual void		closeScreensaver() = 0;

	//! Activate/deactivate screen saver
	/*!
	Forcibly activate the screen saver if \c activate is true otherwise
	forcibly deactivate it.
	*/
	virtual void		screensaver(bool activate) = 0;

	//! Attach to desktop
	/*!
	Called to ensure that this thread is attached to the visible desktop.
	This is mainly intended for microsoft windows which has an artificial
	distinction between desktops where a thread cannot interact with the
	visible desktop unless the thread is attached to that desktop.  Since
	it doesn't report when the visible desktop changes we must poll.
	*/
	virtual void		syncDesktop() = 0;

	//@}
	//! @name accessors
	//@{

	//! Get clipboard
	/*!
	Save the contents of the clipboard indicated by \c id and return
	true iff successful.
	*/
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const = 0;

	//! Get screen shape
	/*!
	Return the position of the upper-left corner of the screen in \c x and
	\c y and the size of the screen in \c w (width) and \c h (height).
	*/
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& w, SInt32& h) const = 0;

	//! Get cursor position
	/*!
	Return the current position of the cursor in \c x and \c y.
	*/
	virtual void		getCursorPos(SInt32& x, SInt32& y) const = 0;

	//! Get cursor center position
	/*!
	Return the cursor center position which is where we park the
	cursor to compute cursor motion deltas and should be far from
	the edges of the screen, typically the center.
	*/
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const = 0;

	//@}
};

#endif
