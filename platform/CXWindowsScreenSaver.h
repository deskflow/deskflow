#ifndef CXWINDOWSSCREENSAVER_H
#define CXWINDOWSSCREENSAVER_H

#include "IScreenSaver.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class CXWindowsScreenSaver : public IScreenSaver {
public:
	// note -- the caller must ensure that Display* passed to c'tor isn't
	// being used in another call to Xlib when calling any method on this
	// object (including during the c'tor and d'tor) except processEvent().
	CXWindowsScreenSaver(Display*);
	virtual ~CXWindowsScreenSaver();

	// process X event.  returns true if the event was handled.
	bool				processEvent(XEvent*);

	// tells this object to send a ClientMessage to the given window
	// when the screen saver activates or deactivates.  only one
	// window can be notified.  the message type is the "SCREENSAVER"
	// atom, the format is 32, and the data.l[0] member is non-zero
	// if activated, zero if deactivated.  pass in None to disable
	// notification;
	void				setNotify(Window);

	// IScreenSaver overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		activate();
	virtual void		deactivate();
	virtual bool		isActive() const;

private:
	// send a notification
	void				sendNotify(bool activated);

	// set xscreensaver's activation state flag.  sends notification
	// if the state has changed.
	void				setXScreenSaver(bool activated);

	// find the running xscreensaver's window
	void				updateXScreenSaver();

	// send a command to xscreensaver
	void				sendXScreenSaverCommand(Atom, long = 0, long = 0) const;

private:
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

	// atoms used to communicate with xscreensaver's window
	Atom				m_atomScreenSaver;
	Atom				m_atomScreenSaverVersion;
	Atom				m_atomScreenSaverActivate;
	Atom				m_atomScreenSaverDeactivate;

	// built-in screen saver settings
	int					m_timeout;
	int					m_interval;
	int					m_preferBlanking;
	int					m_allowExposures;
};

#endif
