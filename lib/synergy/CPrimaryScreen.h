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

#ifndef CPRIMARYSCREEN_H
#define CPRIMARYSCREEN_H

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "OptionTypes.h"
#include "CMutex.h"

class IClipboard;
class IScreen;
class IScreenReceiver;

//! Generic server-side screen
/*!
This is a platform independent base class for primary screen
implementations.  A primary screen is a server-side screen.
Each platform will derive a class from CPrimaryScreen to handle
platform dependent operations.
*/
class CPrimaryScreen {
public:
	CPrimaryScreen(IScreenReceiver*);
	virtual ~CPrimaryScreen();

	//! @name manipulators
	//@{

	//! Open screen
	/*!
	Opens the screen.  This includes initializing the screen, opening
	the screen saver, synchronizing keyboard state, and causing events
	to be reported to an IPrimaryScreenReceiver (set through another
	interface).  Calls close() before returning (rethrowing) if it
	fails for any reason.
	*/
	void				open();

	//! Run event loop
	/*!
	Run the screen's event loop.  This returns when it detects
	the application should terminate or when exitMainLoop() is called.
	mainLoop() may only be called between open() and close().
	*/
	void				mainLoop();

	//! Exit event loop
	/*!
	Force mainLoop() to return.  This call can return before
	mainLoop() does (i.e. asynchronously).
	*/
	void				exitMainLoop();

	//! Close screen
	/*!
	Closes the screen.  This close the screen saver and the screen.
	*/
	void				close();

	//! Enter screen
	/*!
	Called when the user navigates to the primary screen.  Warps
	the cursor to the absolute coordinates \c x,y and unhides
	it.  If \c forScreensaver is true then we're entering because
	the screen saver started and the cursor is not warped.
	*/
	void				enter(SInt32 x, SInt32 y, bool forScreensaver);

	//! Leave screen
	/*!
	Called when the user navigates off the primary screen.  Returns
	true iff successful.
	*/
	bool				leave();

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

	//! Set clipboard
	/*!
	Sets the system's clipboard contents.  This is usually called
	soon after an enter().
	*/
	void				setClipboard(ClipboardID, const IClipboard*);

	//! Grab clipboard
	/*!
	Grabs (i.e. take ownership of) the system clipboard.
	*/
	void				grabClipboard(ClipboardID);

	//! Notify of options changes
	/*!
	Reset all options to their default values.
	*/
	virtual void		resetOptions() = 0;

	//! Notify of options changes
	/*!
	Set options to given values.  Ignore unknown options and don't
	modify our options that aren't given in \c options.
	*/
	virtual void		setOptions(const COptionsList& options) = 0;

	//! Install a one-shot timer
	/*!
	Installs a one-shot timer for \c timeout seconds and returns the
	id of the timer.
	*/
	virtual UInt32		addOneShotTimer(double timeout) = 0;

	//@}
	//! @name accessors
	//@{

	//! Test if active
	/*!
	Returns true iff the screen is active (i.e. the user has left
	the screen).  Note this is the reverse of a secdonary screen.
	*/
	bool				isActive() const;

	//! Get clipboard
	/*!
	Saves the contents of the system clipboard indicated by \c id.
	*/
	void				getClipboard(ClipboardID, IClipboard*) const;

	//! Get jump zone size
	/*!
	Return the jump zone size, the size of the regions on the edges of
	the screen that cause the cursor to jump to another screen.
	*/
	virtual SInt32		getJumpZoneSize() const = 0;

	//! Get toggle key state
	/*!
	Return the primary screen's current toggle modifier key state.
	The returned mask should have the corresponding bit set for
	each toggle key that is active.  For example, if caps lock is
	on then the returned mask should have \c KeyModifierCapsLock set.
	*/
	virtual KeyModifierMask	getToggleMask() const = 0;

	//! Get screen lock state
	/*!
	Return true if any key or button is being pressed or if there's
	any other reason that the user should not be allowed to switch
	screens.  Active toggle keys (including the scroll lock key)
	should not be counted as reasons to lock to the screen.
	If this method returns true it should log a message on why at
	the CLOG_DEBUG level.
	*/
	virtual bool		isLockedToScreen() const = 0;

	//! Get screen
	/*!
	Return the platform dependent screen.
	*/
	virtual IScreen*	getScreen() const = 0;

	//@}

protected:
	//! Pre-mainLoop() hook
	/*!
	Called on entry to mainLoop().  Override to perform platform specific
	operations.  Default does nothing.  May throw.
	*/
	virtual void		onPreMainLoop();

	//! Post-mainLoop() hook
	/*!
	Called on exit from mainLoop().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostMainLoop();

	//! Pre-open() hook
	/*!
	Called on entry to open().  Override to perform platform specific
	operations.  Default does nothing.  May throw.
	*/
	virtual void		onPreOpen();

	//! Post-open() hook
	/*!
	Called on exit from open() iff the open was successful.  Default
	does nothing.  May throw.
	*/
	virtual void		onPostOpen();

	//! Pre-close() hook
	/*!
	Called on entry to close().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPreClose();

	//! Post-close() hook
	/*!
	Called on exit from close().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostClose();

	//! Pre-enter() hook
	/*!
	Called from enter() after the cursor has been warped or, if
	\c forScreensaver is true, onEnterScreensaver() was called.  Override
	to perform platform specific operations.  Default does nothing.  May
	\b not throw.
	*/
	virtual void		onPreEnter();

	//! Post-enter() hook
	/*!
	Called on exit from enter().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostEnter();

	//! Pre-enter() for screen saver hook
	/*!
	Called on entry to enter() if the \c forScreensaver passed to it was
	true.  Override to perform platform specific operations.  Default
	does nothing.  May \b not throw.
	*/
	virtual void		onEnterScreensaver();

	//! Pre-leave() hook
	/*!
	Called on entry to leave() after desktop synchronization.  Override
	to perform platform specific operations.  Default does nothing.  May
	\b not throw.
	*/
	virtual void		onPreLeave();

	//! Post-leave() hook
	/*!
	Called on exit from leave().  \c success is the value returned by
	showWindow().  Override to perform platform specific operations.
	Default does nothing.  May \b not throw.
	*/
	virtual void		onPostLeave(bool success);

	//! Create window
	/*!
	Called to create the window.  This window is generally used to
	receive events, hide the cursor, and to capture keyboard and mouse
	input.
	*/
	virtual void		createWindow() = 0;

	//! Destroy window
	/*!
	Called to destroy the window created by createWindow().
	*/
	virtual void		destroyWindow() = 0;

	//! Show window
	/*!
	Called when the user navigates off the primary screen.  Hide the
	cursor and grab exclusive access to the input devices.  Every call
	to showWindow() has a matching call to hideWindow() which preceeds
	it.  Return true iff successful (in particular, iff the input
	devices were grabbed).
	
	After a successful showWindow(), user input events and
	screensaver activation/deactivation should be reported to an
	IPrimaryScreenReceiver (set through another interface) until
	hideWindow() is called.  Report mouse motion to its
	onMouseMoveSecondary().  User input should not be delivered to
	any application except this one.
	*/
	virtual bool		showWindow() = 0;

	//! Hide window
	/*!
	Called when the user navigates back to the primary screen.  Show
	the cursor and ungrab the input devices.
	
	After hideWindow(), user input events should be delivered normally
	to other applications.  Mouse motion over (at least) the jump zones
	must be reported to an IPrimaryScreenReceiver's onMouseMovePrimary().
	*/
	virtual void		hideWindow() = 0;

	//! Warp cursor for relative motion
	/*!
	Prepare the cursor to report relative motion.  When the user has
	navigated to another screen, synergy requires the cursor motion
	deltas, not the absolute coordinates.  Typically this is done by
	warping the cursor to the center of the primary screen and then
	every time it moves compute the motion and warp back to the
	center (but without reporting that warp as motion).  This is
	only called after a successful showWindow().
	*/
	virtual void		warpCursorToCenter() = 0;

	//! Synchronize key state
	/*!
	Check the current keyboard state.  Normally a screen will save
	the keyboard state in this method and use this shadow state
	when handling user input and in methods like isLockedToScreen().
	*/
	virtual void		updateKeys() = 0;

private:
	void				enterNoWarp();

private:
	CMutex				m_mutex;

	// object to notify of changes
	IScreenReceiver*	m_receiver;

	// m_active is true if this screen has been left
	bool				m_active;
};

#endif
