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

#ifndef CXWINDOWSSCREEN_H
#define CXWINDOWSSCREEN_H

#include "IScreen.h"
#include "CMutex.h"
#include "CStopwatch.h"
#include "stdvector.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif
#include <algorithm>
#include <functional>

class IJob;
class IScreenEventHandler;
class IScreenReceiver;
class CXWindowsClipboard;
class CXWindowsScreenSaver;

/*!
\class CEvent
\brief User event data
An architecture dependent type holding user event data.
*/
// X11 event
class CEvent {
public:
	XEvent				m_event;
	SInt32				m_result;
};

//! Implementation of IScreen for X11
class CXWindowsScreen : public IScreen {
public:
	CXWindowsScreen(IScreenReceiver*, IScreenEventHandler*);
	virtual ~CXWindowsScreen();

	//! @name manipulators
	//@{

	//! Add timer
	/*!
	Add a job to invoke every timeout seconds.  The job is
	called with the display locked.  If a job timeout expires twice
	or more before the job can be called then the job is called
	just once.  The caller retains ownership of the job.
	*/
	void				addTimer(IJob*, double timeout);

	//! Remove timer
	/*!
	Remove a job.  The caller retains ownership of the job.
	*/
	void				removeTimer(IJob*);

	//! Install a one-shot timer
	/*!
	Installs a one-shot timer for \c timeout seconds and returns the
	id of the timer (which will be passed to the receiver's
	\c onTimerExpired()).
	*/
	UInt32				addOneShotTimer(double timeout);

	//! Set window
	/*!
	Set the window (created by the subclass).  This performs some
	initialization and saves the window in case it's needed later.
	*/
	void				setWindow(Window);

	//@}
	//! @name accessors
	//@{

	//! Get window
	/*!
	Returns the root window of the screen.
	*/
	Window				getRoot() const;

	//! Get transparent cursor
	/*!
	Returns a cursor that is transparent everywhere.
	*/
	Cursor				getBlankCursor() const;

	//@}

	// IScreen overrides
	void				open();
	void				mainLoop();
	void				exitMainLoop();
	void				close();
	bool				setClipboard(ClipboardID, const IClipboard*);
	void				checkClipboards();
	void				openScreensaver(bool notify);
	void				closeScreensaver();
	void				screensaver(bool activate);
	void				syncDesktop();
	bool				getClipboard(ClipboardID, IClipboard*) const;
	void				getShape(SInt32&, SInt32&, SInt32&, SInt32&) const;
	void				getCursorPos(SInt32&, SInt32&) const;
	void				getCursorCenter(SInt32&, SInt32&) const;

private:
	// update screen size cache
	void				updateScreenShape();

	// process events before dispatching to receiver
	bool				onPreDispatch(CEvent* event);

	// create the transparent cursor
	void				createBlankCursor();

	// remove a timer without locking
	void				removeTimerNoLock(IJob*);

	// process timers
	bool				processTimers();

	// determine the clipboard from the X selection.  returns
	// kClipboardEnd if no such clipboard.
	ClipboardID			getClipboardID(Atom selection) const;

	// continue processing a selection request
	void				processClipboardRequest(Window window,
							Time time, Atom property);

	// terminate a selection request
	void				destroyClipboardRequest(Window window);

	// X I/O error handler
	static int			ioErrorHandler(Display*);

private:
	// a priority queue will direct access to the elements
	template <class T, class Container = std::vector<T>,
				class Compare = std::greater<typename Container::value_type> >
	class CPriorityQueue {
	public:
		typedef typename Container::value_type value_type;
		typedef typename Container::size_type size_type;
		typedef typename Container::iterator iterator;
		typedef Container container_type;

		CPriorityQueue() { }
		CPriorityQueue(Container& swappedIn);
		~CPriorityQueue() { }

		// manipulators

		void			push(const value_type& v)
		{
			c.push_back(v);
			std::push_heap(c.begin(), c.end(), comp);
		}

		void			pop()
		{
			std::pop_heap(c.begin(), c.end(), comp);
			c.pop_back();
		}

		iterator		begin()
		{
			return c.begin();
		}

		iterator		end()
		{
			return c.end();
		}

		void			swap(CPriorityQueue<T, Container, Compare>& q)
		{
			c.swap(q.c);
		}

		void			swap(Container& c2)
		{
			c.swap(c2);
			std::make_heap(c.begin(), c.end(), comp);
		}

		// accessors

		bool			empty() const
		{
			return c.empty();
		}

		size_type		size() const
		{
			return c.size();
		}

		const value_type&
						top() const
		{
			return c.front();
		}

	private:
		Container		c;
		Compare			comp;
	};

	// a timer priority queue element
	class CTimer {
	public:
		CTimer(IJob* job, double startTime, double resetTime);
		~CTimer();

		// manipulators

		void			run();

		void			reset();

		CTimer&			operator-=(double);

		// accessors

		IJob*			getJob() const
		{
			return m_job;
		}

		operator double() const;

		bool			operator<(const CTimer&) const;

	private:
		IJob*			m_job;
		double			m_timeout;
		double			m_time;
		double			m_startTime;
	};

private:
	friend class CDisplayLock;

	typedef CPriorityQueue<CTimer> CTimerPriorityQueue;

	// X is not thread safe
	CMutex				m_mutex;

	Display*			m_display;
	Window				m_root;
	bool				m_stop;

	IScreenReceiver*		m_receiver;
	IScreenEventHandler*	m_eventHandler;
	Window				m_window;

	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// clipboards
	CXWindowsClipboard*	m_clipboard[kClipboardEnd];

	// the transparent cursor
	Cursor				m_cursor;

	// screen saver stuff
	CXWindowsScreenSaver*	m_screensaver;
	bool				m_screensaverNotify;
	Atom				m_atomScreensaver;

	// timers, the stopwatch used to time, and a mutex for the timers
	CTimerPriorityQueue	m_timers;
	CStopwatch			m_time;
	CMutex				m_timersMutex;
	CTimer*				m_oneShotTimer;

	// pointer to (singleton) screen.  this is only needed by
	// ioErrorHandler().
	static CXWindowsScreen*	s_screen;
};

//! Convenience object to lock/unlock a CXWindowsScreen
class CDisplayLock {
public:
	CDisplayLock(const CXWindowsScreen*);
	~CDisplayLock();

	operator Display*() const;

private:
	const CMutex*		m_mutex;
	Display*			m_display;
};

#endif
