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

class CXWindowsScreenSaver : public IScreenSaver {
public:
	// note -- the caller must ensure that Display* passed to c'tor isn't
	// being used in another call to Xlib when calling any method on this
	// object (including during the c'tor and d'tor).
	CXWindowsScreenSaver(CXWindowsScreen*, Display*);
	virtual ~CXWindowsScreenSaver();

	// called for each event before event translation and dispatch.  return
	// true to skip translation and dispatch.  subclasses should call the
	// superclass's version first and return true if it returns true.
	bool				onPreDispatch(const XEvent*);

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

	// built-in screen saver settings
	int					m_timeout;
	int					m_interval;
	int					m_preferBlanking;
	int					m_allowExposures;

	// true iff the disabled job timer is installed
	bool				m_disabled;

	// the job used to invoke disableCallback
	IJob*				m_disableJob;
};

#endif
