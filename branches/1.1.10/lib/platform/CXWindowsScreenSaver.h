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

#ifndef CXWINDOWSSCREENSAVER_H
#define CXWINDOWSSCREENSAVER_H

#include "IScreenSaver.h"
#include "stdmap.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class IJob;
class CXWindowsScreen;

//! X11 screen saver implementation
class CXWindowsScreenSaver : public IScreenSaver {
public:
	// note -- the caller must ensure that Display* passed to c'tor isn't
	// being used in another call to Xlib when calling any method on this
	// object (including during the c'tor and d'tor).
	CXWindowsScreenSaver(CXWindowsScreen*, Display*);
	virtual ~CXWindowsScreenSaver();

	//! @name manipulators
	//@{

	//! Event filtering
	/*!
	Called for each event before event translation and dispatch.  Return
	true to skip translation and dispatch.  Subclasses should call the
	superclass's version first and return true if it returns true.
	*/
	bool				onPreDispatch(const XEvent*);

	//! Set notify target
	/*!
	Tells this object to send a ClientMessage to the given window
	when the screen saver activates or deactivates.  Only one window
	can be notified at a time.  The message type is the "SCREENSAVER"
	atom, the format is 32, and the data.l[0] member is non-zero
	if activated, zero if deactivated.  Pass None to disable
	notification.
	*/
	void				setNotify(Window);

	//@}

	// IScreenSaver overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		activate();
	virtual void		deactivate();
	virtual bool		isActive() const;

private:
	// send a notification
	void				sendNotify(bool activated);

	// find and set the running xscreensaver's window.  returns true iff
	// found.
	bool				findXScreenSaver();

	// set the xscreensaver's window, updating the activation state flag
	void				setXScreenSaver(Window);

	// returns true if the window appears to be the xscreensaver window
	bool				isXScreenSaver(Window) const;

	// set xscreensaver's activation state flag.  sends notification
	// if the state has changed.
	void				setXScreenSaverActive(bool activated);

	// send a command to xscreensaver
	void				sendXScreenSaverCommand(Atom, long = 0, long = 0);

	// watch all windows that could potentially be the xscreensaver for
	// the events that will confirm it.
	void				watchForXScreenSaver();

	// stop watching all watched windows
	void				clearWatchForXScreenSaver();

	// add window to the watch list
	void				addWatchXScreenSaver(Window window);

	// install/uninstall the job used to suppress the screensaver
	void				updateDisableJob();

	// called periodically to prevent the screen saver from starting
	void				disableCallback(void*);

private:
	typedef std::map<Window, long> CWatchList;

	// the event loop object
	CXWindowsScreen*	m_screen;

	// the X display
	Display*			m_display;

	// old event mask on root window
	long				m_rootEventMask;

	// window to notify on screen saver activation/deactivation
	Window				m_notify;

	// xscreensaver's window
	Window				m_xscreensaver;

	// xscreensaver activation state
	bool				m_xscreensaverActive;

	// dummy window to receive xscreensaver repsonses
	Window				m_xscreensaverSink;

	// potential xscreensaver windows being watched
	CWatchList			m_watchWindows;

	// atoms used to communicate with xscreensaver's window
	Atom				m_atomScreenSaver;
	Atom				m_atomScreenSaverVersion;
	Atom				m_atomScreenSaverActivate;
	Atom				m_atomScreenSaverDeactivate;
	Atom				m_atomSynergyScreenSaver;

	// built-in screen saver settings
	int					m_timeout;
	int					m_interval;
	int					m_preferBlanking;
	int					m_allowExposures;

	// true iff the client wants the screen saver suppressed
	bool				m_disabled;

	// true iff we're ignoring m_disabled.  this is true, for example,
	// when the client has called activate() and so presumably wants
	// to activate the screen saver even if disabled.
	bool				m_suppressDisable;

	// true iff the disabled job timer is installed
	bool				m_disableJobInstalled;

	// the job used to invoke disableCallback
	IJob*				m_disableJob;
};

#endif
