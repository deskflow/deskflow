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

#include "CXWindowsScreenSaver.h"
#include "CXWindowsScreen.h"
#include "CXWindowsUtil.h"
#include "CLog.h"
#include "TMethodJob.h"
#include <X11/Xatom.h>
#if defined(HAVE_X11_EXTENSIONS_XTEST_H)
#	include <X11/extensions/XTest.h>
#else
#	error The XTest extension is required to build synergy
#endif

//
// CXWindowsScreenSaver
//

CXWindowsScreenSaver::CXWindowsScreenSaver(
				CXWindowsScreen* screen, Display* display) :
	m_screen(screen),
	m_display(display),
	m_notify(None),
	m_xscreensaver(None),
	m_xscreensaverActive(false),
	m_disabled(false),
	m_suppressDisable(false),
	m_disableJobInstalled(false)
{
	// screen saver disable callback
	m_disableJob = new TMethodJob<CXWindowsScreenSaver>(this,
								&CXWindowsScreenSaver::disableCallback);

	// get atoms
	m_atomScreenSaver           = XInternAtom(m_display,
										"SCREENSAVER", False);
	m_atomScreenSaverVersion    = XInternAtom(m_display,
										"_SCREENSAVER_VERSION", False);
	m_atomScreenSaverActivate   = XInternAtom(m_display,
										"ACTIVATE", False);
	m_atomScreenSaverDeactivate = XInternAtom(m_display,
										"DEACTIVATE", False);
	m_atomSynergyScreenSaver    = XInternAtom(m_display,
										"SYNERGY_SCREENSAVER", False);

	// create dummy window to receive xscreensaver responses.  this
	// shouldn't be necessary (we should be able to send responses
	// to None) but it doesn't hurt.
	XSetWindowAttributes attr;
	attr.event_mask            = 0;//PropertyChangeMask;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect     = True;
	m_xscreensaverSink = XCreateWindow(m_display,
								DefaultRootWindow(m_display),
								0, 0, 1, 1, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect,
								&attr);
	LOG((CLOG_DEBUG "xscreensaver sink window is 0x%08x", m_xscreensaverSink));

	// watch top-level windows for changes
	{
		bool error = false;
		CXWindowsUtil::CErrorLock lock(m_display, &error);
		Window root = DefaultRootWindow(m_display);
		XWindowAttributes attr;
		XGetWindowAttributes(m_display, root, &attr);
		m_rootEventMask = attr.your_event_mask;
		XSelectInput(m_display, root, m_rootEventMask | SubstructureNotifyMask);
		if (error) {
			LOG((CLOG_DEBUG "didn't set root event mask"));
			m_rootEventMask = 0;
		}
	}

	// get the xscreensaver window, if any
	if (!findXScreenSaver()) {
		setXScreenSaver(None);
	}

	// get the built-in settings
	XGetScreenSaver(m_display, &m_timeout, &m_interval,
								&m_preferBlanking, &m_allowExposures);
}

CXWindowsScreenSaver::~CXWindowsScreenSaver()
{
	// clear watch list
	clearWatchForXScreenSaver();

	// stop watching root for events
	CXWindowsUtil::CErrorLock lock(m_display);
	Window root = DefaultRootWindow(m_display);
	XSelectInput(m_display, root, m_rootEventMask);

	// destroy dummy sink window
	XDestroyWindow(m_display, m_xscreensaverSink);

	// done with disable job
	m_screen->removeTimer(m_disableJob);
	delete m_disableJob;
}

bool
CXWindowsScreenSaver::onPreDispatch(const XEvent* xevent)
{
	switch (xevent->type) {
	case CreateNotify:
		if (m_xscreensaver == None) {
			if (isXScreenSaver(xevent->xcreatewindow.window)) {
				// found the xscreensaver
				setXScreenSaver(xevent->xcreatewindow.window);
			}
			else {
				// another window to watch.  to detect the xscreensaver
				// window we look for a property but that property may
				// not yet exist by the time we get this event so we
				// have to watch the window for property changes.
				// this would be so much easier if xscreensaver did the
				// smart thing and stored its window in a property on
				// the root window.
				addWatchXScreenSaver(xevent->xcreatewindow.window);
			}
		}
		break;

	case DestroyNotify:
		if (xevent->xdestroywindow.window == m_xscreensaver) {
			// xscreensaver is gone
			LOG((CLOG_DEBUG "xscreensaver died"));
			setXScreenSaver(None);
			return true;
		}
		break;

	case PropertyNotify:
		if (xevent->xproperty.state == PropertyNewValue) {
			if (isXScreenSaver(xevent->xproperty.window)) {
				// found the xscreensaver
				setXScreenSaver(xevent->xcreatewindow.window);
			}
		}
		break;

	case MapNotify:
		if (xevent->xmap.window == m_xscreensaver) {
			// xscreensaver has activated
			setXScreenSaverActive(true);
			return true;
		}
		break;

	case UnmapNotify:
		if (xevent->xunmap.window == m_xscreensaver) {
			// xscreensaver has deactivated
			setXScreenSaverActive(false);
			return true;
		}
		break;
	}

	return false;
}

void
CXWindowsScreenSaver::setNotify(Window notify)
{
	m_notify = notify;
}

void
CXWindowsScreenSaver::enable()
{
	// for xscreensaver
	m_disabled = false;
	updateDisableJob();

	// for built-in X screen saver
	XSetScreenSaver(m_display, m_timeout, m_interval,
								m_preferBlanking, m_allowExposures);
}

void
CXWindowsScreenSaver::disable()
{
	// for xscreensaver
	m_disabled = true;
	updateDisableJob();

	// use built-in X screen saver
	XGetScreenSaver(m_display, &m_timeout, &m_interval,
								&m_preferBlanking, &m_allowExposures);
	XSetScreenSaver(m_display, 0, m_interval,
								m_preferBlanking, m_allowExposures);
	// FIXME -- now deactivate?
}

void
CXWindowsScreenSaver::activate()
{
	// remove disable job timer
	m_suppressDisable = true;
	updateDisableJob();

	// try xscreensaver
	findXScreenSaver();
	if (m_xscreensaver != None) {
		sendXScreenSaverCommand(m_atomScreenSaverActivate);
		return;
	}

	// use built-in X screen saver
	XForceScreenSaver(m_display, ScreenSaverActive);
}

void
CXWindowsScreenSaver::deactivate()
{
	// reinstall disable job timer
	m_suppressDisable = false;
	updateDisableJob();

	// try xscreensaver
	findXScreenSaver();
	if (m_xscreensaver != None) {
		sendXScreenSaverCommand(m_atomScreenSaverDeactivate);
		return;
	}

	// use built-in X screen saver
	XForceScreenSaver(m_display, ScreenSaverReset);
}

bool
CXWindowsScreenSaver::isActive() const
{
	// check xscreensaver
	if (m_xscreensaver != None) {
		return m_xscreensaverActive;
	}

	// can't check built-in X screen saver activity
	return false;
}

void
CXWindowsScreenSaver::sendNotify(bool activated)
{
	if (m_notify != None) {
		XEvent event;
		event.xclient.type         = ClientMessage;
		event.xclient.display      = m_display;
		event.xclient.window       = m_notify;
		event.xclient.message_type = m_atomSynergyScreenSaver;
		event.xclient.format       = 32;
		event.xclient.data.l[0]    = activated ? 1 : 0;
		event.xclient.data.l[1]    = 0;
		event.xclient.data.l[2]    = 0;
		event.xclient.data.l[3]    = 0;
		event.xclient.data.l[4]    = 0;

		CXWindowsUtil::CErrorLock lock(m_display);
		XSendEvent(m_display, m_notify, False, 0, &event);
	}
}

bool
CXWindowsScreenSaver::findXScreenSaver()
{
	// do nothing if we've already got the xscreensaver window
	if (m_xscreensaver == None) {
		// find top-level window xscreensaver window
		Window root = DefaultRootWindow(m_display);
		Window rw, pw, *cw;
		unsigned int nc;
		if (XQueryTree(m_display, root, &rw, &pw, &cw, &nc)) {
			for (unsigned int i = 0; i < nc; ++i) {
				if (isXScreenSaver(cw[i])) {
					setXScreenSaver(cw[i]);
					break;
				}
			}
			XFree(cw);
		}
	}

	return (m_xscreensaver != None);
}

void
CXWindowsScreenSaver::setXScreenSaver(Window window)
{
	LOG((CLOG_DEBUG "xscreensaver window: 0x%08x", window));

	// save window
	m_xscreensaver = window;

	if (m_xscreensaver != None) {
		// clear old watch list
		clearWatchForXScreenSaver();

		// see if xscreensaver is active
		bool error = false;
		CXWindowsUtil::CErrorLock lock(m_display, &error);
		XWindowAttributes attr;
		XGetWindowAttributes(m_display, m_xscreensaver, &attr);
		setXScreenSaverActive(!error && attr.map_state != IsUnmapped);
	}
	else {
		// screen saver can't be active if it doesn't exist
		setXScreenSaverActive(false);

		// start watching for xscreensaver
		watchForXScreenSaver();
	}
}

bool
CXWindowsScreenSaver::isXScreenSaver(Window w) const
{
	// check for m_atomScreenSaverVersion string property
	Atom type;
	return (CXWindowsUtil::getWindowProperty(m_display, w,
									m_atomScreenSaverVersion,
									NULL, &type, NULL, False) &&
								type == XA_STRING);
}

void
CXWindowsScreenSaver::setXScreenSaverActive(bool activated)
{
	if (m_xscreensaverActive != activated) {
		LOG((CLOG_DEBUG "xscreensaver %s on window 0x%08x", activated ? "activated" : "deactivated", m_xscreensaver));
		m_xscreensaverActive = activated;

		// if screen saver was activated forcefully (i.e. against
		// our will) then just accept it.  don't try to keep it
		// from activating since that'll just pop up the password
		// dialog if locking is enabled.
		m_suppressDisable = activated;
		updateDisableJob();

		sendNotify(activated);
	}
}

void
CXWindowsScreenSaver::sendXScreenSaverCommand(Atom cmd, long arg1, long arg2)
{
	XEvent event;
	event.xclient.type         = ClientMessage;
	event.xclient.display      = m_display;
	event.xclient.window       = m_xscreensaverSink;
	event.xclient.message_type = m_atomScreenSaver;
	event.xclient.format       = 32;
	event.xclient.data.l[0]    = static_cast<long>(cmd);
	event.xclient.data.l[1]    = arg1;
	event.xclient.data.l[2]    = arg2;
	event.xclient.data.l[3]    = 0;
	event.xclient.data.l[4]    = 0;

	LOG((CLOG_DEBUG "send xscreensaver command: %d %d %d", (long)cmd, arg1, arg2));
	bool error = false;
	CXWindowsUtil::CErrorLock lock(m_display, &error);
	XSendEvent(m_display, m_xscreensaver, False, 0, &event);
	if (error) {
		findXScreenSaver();
	}
}

void
CXWindowsScreenSaver::watchForXScreenSaver()
{
	// clear old watch list
	clearWatchForXScreenSaver();

	// add every child of the root to the list of windows to watch
	Window root = DefaultRootWindow(m_display);
	Window rw, pw, *cw;
	unsigned int nc;
	if (XQueryTree(m_display, root, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i) {
			addWatchXScreenSaver(cw[i]);
		}
		XFree(cw);
	}

	// now check for xscreensaver window in case it set the property
	// before we could request property change events.
	if (findXScreenSaver()) {
		// found it so clear out our watch list
		clearWatchForXScreenSaver();
	}
}

void
CXWindowsScreenSaver::clearWatchForXScreenSaver()
{
	// stop watching all windows
	CXWindowsUtil::CErrorLock lock(m_display);
	for (CWatchList::iterator index = m_watchWindows.begin();
								index != m_watchWindows.end(); ++index) {
		XSelectInput(m_display, index->first, index->second);
	}
	m_watchWindows.clear();
}

void
CXWindowsScreenSaver::addWatchXScreenSaver(Window window)
{
	bool error = false;
	CXWindowsUtil::CErrorLock lock(m_display, &error);

	// get window attributes
	XWindowAttributes attr;
	XGetWindowAttributes(m_display, window, &attr);

	// if successful and window uses override_redirect (like xscreensaver
	// does) then watch it for property changes.  
	if (!error && attr.override_redirect == True) {
		XSelectInput(m_display, window,
								attr.your_event_mask | PropertyChangeMask);
		if (!error) {
			// if successful then add the window to our list
			m_watchWindows.insert(std::make_pair(window, attr.your_event_mask));
		}
	}
}

void
CXWindowsScreenSaver::updateDisableJob()
{
	assert(m_disableJob != NULL);

	if (m_disabled && !m_suppressDisable && !m_disableJobInstalled) {
		// 5 seconds should be plenty often to suppress the screen saver
		m_disableJobInstalled = true;
		m_screen->addTimer(m_disableJob, 5.0);
	}
	else if ((!m_disabled || m_suppressDisable) && m_disableJobInstalled) {
		m_disableJobInstalled = false;
		m_screen->removeTimer(m_disableJob);
	}
}

void
CXWindowsScreenSaver::disableCallback(void*)
{
	// send fake mouse motion directly to xscreensaver
	if (m_xscreensaver != None) {
		XEvent event;
		event.xmotion.type         = MotionNotify;
		event.xmotion.display      = m_display;
		event.xmotion.window       = m_xscreensaver;
		event.xmotion.root         = DefaultRootWindow(m_display);
		event.xmotion.subwindow    = None;
		event.xmotion.time         = CurrentTime;
		event.xmotion.x            = 0;
		event.xmotion.y            = 0;
		event.xmotion.x_root       = 0;
		event.xmotion.y_root       = 0;
		event.xmotion.state        = 0;
		event.xmotion.is_hint      = NotifyNormal;
		event.xmotion.same_screen  = True;

		CXWindowsUtil::CErrorLock lock(m_display);
		XSendEvent(m_display, m_xscreensaver, False, 0, &event);
	}
}
