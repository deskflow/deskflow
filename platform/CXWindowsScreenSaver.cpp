#include "CXWindowsScreenSaver.h"
#include "CXWindowsUtil.h"
#include <X11/Xatom.h>

//
// CXWindowsScreenSaver
//

CXWindowsScreenSaver::CXWindowsScreenSaver(Display* display) :
	m_display(display),
	m_notify(None),
	m_xscreensaver(None)
{
	// get atoms
	m_atomScreenSaver           = XInternAtom(m_display,
										"SCREENSAVER", False);
	m_atomScreenSaverVersion    = XInternAtom(m_display,
										"_SCREENSAVER_VERSION", False);
	m_atomScreenSaverActivate   = XInternAtom(m_display,
										"ACTIVATE", False);
	m_atomScreenSaverDeactivate = XInternAtom(m_display,
										"DEACTIVATE", False);

	// watch top-level windows for changes
	{
		bool error = false;
		CXWindowsUtil::CErrorLock lock(&error);
		Window root = DefaultRootWindow(m_display);
		XWindowAttributes attr;
		XGetWindowAttributes(m_display, root, &attr);
		m_rootEventMask = attr.your_event_mask;
		XSelectInput(m_display, root, m_rootEventMask | SubstructureNotifyMask);
		if (error) {
			m_rootEventMask = 0;
		}
	}

	// get the xscreensaver window, if any
	updateXScreenSaver();

	// get the built-in settings
	XGetScreenSaver(m_display, &m_timeout, &m_interval,
								&m_preferBlanking, &m_allowExposures);
}

CXWindowsScreenSaver::~CXWindowsScreenSaver()
{
	// stop watching root for events
	CXWindowsUtil::CErrorLock lock;
	Window root = DefaultRootWindow(m_display);
	XSelectInput(m_display, root, m_rootEventMask);
}

bool
CXWindowsScreenSaver::processEvent(XEvent* xevent)
{
	switch (xevent->type) {
	case DestroyNotify:
		if (xevent->xdestroywindow.window == m_xscreensaver) {
			// xscreensaver is gone
			setXScreenSaver(false);
			m_xscreensaver = None;
			return true;
		}
		break;

	case MapNotify:
		if (xevent->xmap.window == m_xscreensaver) {
			// xscreensaver has activated
			setXScreenSaver(true);
			return true;
		}
		break;

	case UnmapNotify:
		if (xevent->xunmap.window == m_xscreensaver) {
			// xscreensaver has deactivated
			setXScreenSaver(false);
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
	// try xscreensaver
	updateXScreenSaver();
	if (m_xscreensaver) {
		// FIXME
		return;
	}

	// use built-in X screen saver
	XSetScreenSaver(m_display, m_timeout, m_interval,
								m_preferBlanking, m_allowExposures);
}

void
CXWindowsScreenSaver::disable()
{
	// try xscreensaver
	updateXScreenSaver();
	if (m_xscreensaver) {
		// FIXME
		return;
	}

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
	// try xscreensaver
	updateXScreenSaver();
	if (m_xscreensaver) {
		sendXScreenSaverCommand(m_atomScreenSaverActivate);
		return;
	}

	// use built-in X screen saver
	XForceScreenSaver(m_display, ScreenSaverActive);
}

void
CXWindowsScreenSaver::deactivate()
{
	// try xscreensaver
	updateXScreenSaver();
	if (m_xscreensaver) {
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
		event.xclient.message_type = m_atomScreenSaver;
		event.xclient.format       = 32;
		event.xclient.data.l[0]    = activated ? 1 : 0;
		event.xclient.data.l[1]    = 0;
		event.xclient.data.l[2]    = 0;
		event.xclient.data.l[3]    = 0;
		event.xclient.data.l[4]    = 0;

		CXWindowsUtil::CErrorLock lock;
		XSendEvent(m_display, m_notify, False, 0, &event);
	}
}

void
CXWindowsScreenSaver::setXScreenSaver(bool activated)
{
	if (m_xscreensaverActive != activated) {
		m_xscreensaverActive = activated;
		sendNotify(activated);
	}
}

void
CXWindowsScreenSaver::updateXScreenSaver()
{
	// do nothing if we've already got the xscreensaver window
	if (m_xscreensaver != None) {
		return;
	}

	// find top-level window with m_atomScreenSaverVersion string property
	CXWindowsUtil::CErrorLock lock;
	Window root  = DefaultRootWindow(m_display);
	Window rw, pw, *cw;
	unsigned int nc;
	if (XQueryTree(m_display, root, &rw, &pw, &cw, &nc)) {
		CString data;
		Atom type;
		for (unsigned int i = 0; i < nc; ++i) {
			if (CXWindowsUtil::getWindowProperty(m_display, cw[i],
                                   m_atomScreenSaverVersion,
                                   &data, &type, NULL, False) &&
								type == XA_STRING) {
				m_xscreensaver = cw[i];
				break;
			}
		}
		XFree(cw);
	}

	// see if xscreensaver is active
	if (m_xscreensaver != None) {
		bool error = false;
		CXWindowsUtil::CErrorLock lock(&error);
		XWindowAttributes attr;
		XGetWindowAttributes(m_display, m_xscreensaver, &attr);
		setXScreenSaver(!error && attr.map_state != IsUnmapped);
	}
	else {
		setXScreenSaver(false);
	}
}

void
CXWindowsScreenSaver::sendXScreenSaverCommand(
				Atom cmd, long arg1, long arg2) const
{
	XEvent event;
	event.xclient.type         = ClientMessage;
	event.xclient.display      = m_display;
	event.xclient.window       = None;
	event.xclient.message_type = m_atomScreenSaver;
	event.xclient.format       = 32;
	event.xclient.data.l[0]    = static_cast<long>(cmd);
	event.xclient.data.l[1]    = arg1;
	event.xclient.data.l[2]    = arg2;
	event.xclient.data.l[3]    = 0;
	event.xclient.data.l[4]    = 0;

	CXWindowsUtil::CErrorLock lock;
	XSendEvent(m_display, m_xscreensaver, False, 0, &event);
}
