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

#ifndef CXWINDOWSEVENTQUEUE_H
#define CXWINDOWSEVENTQUEUE_H

#include "CEventQueue.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

//! Event queue for X11
class CXWindowsEventQueue : public CEventQueue {
public:
	CXWindowsEventQueue(Display*);
	virtual ~CXWindowsEventQueue();

	//! @name manipulators
	//@{

	//@}
	//! @name accessors
	//@{

	//@}

protected:
	// CEventQueue overrides
	virtual void		waitForEvent(double timeout);
	virtual bool		doGetEvent(CEvent& event);
	virtual bool		doAddEvent(CEvent::Type type, UInt32 dataID);
	virtual bool		doIsEmpty() const;
	virtual CEventQueueTimer*
						doNewTimer(double duration, bool oneShot) const;
	virtual void		doDeleteTimer(CEventQueueTimer*) const;

private:
	void				processSystemEvent(CEvent& event);
	void				processClientMessage(CEvent& event);

private:
	Display*			m_display;
	Window				m_window;
	Atom				m_userEvent;
	XEvent				m_event;
};

#endif
