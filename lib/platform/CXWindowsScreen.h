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

#include "IPlatformScreen.h"
#include "CXWindowsKeyMapper.h"
#include "CMutex.h"
#include "CStopwatch.h"
#include "CPriorityQueue.h"
#include "stdvector.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class CXWindowsClipboard;
class CXWindowsScreenSaver;
class IJob;
class IScreenReceiver;
class IPrimaryScreenReceiver;

//! Implementation of IPlatformScreen for X11
class CXWindowsScreen : public IPlatformScreen {
public:
	CXWindowsScreen(IScreenReceiver*, IPrimaryScreenReceiver*);
	virtual ~CXWindowsScreen();

	//! @name manipulators
	//@{

	//! Add timer
	/*!
	Add a job to invoke every timeout seconds.  The job is called
	with the display locked.  If a job timeout expires twice or
	more before the job can be called then the job is called just
	once.  The caller retains ownership of the job.
	*/
	void				addTimer(IJob*, double timeout);

	//! Remove timer
	/*!
	Remove a job.  The caller retains ownership of the job.
	*/
	void				removeTimer(IJob*);

	//@}

	// IPlatformScreen overrides
	virtual void		open(IKeyState*);
	virtual void		close();
	virtual void		enable();
	virtual void		disable();
	virtual void		mainLoop();
	virtual void		exitMainLoop();
	virtual void		enter();
	virtual bool		leave();
	virtual bool		setClipboard(ClipboardID, const IClipboard*);
	virtual void		checkClipboards();
	virtual void		openScreensaver(bool notify);
	virtual void		closeScreensaver();
	virtual void		screensaver(bool activate);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual void		updateKeys();
	virtual bool		isPrimary() const;
	virtual bool		getClipboard(ClipboardID, IClipboard*) const;
	virtual void		getShape(SInt32&, SInt32&, SInt32&, SInt32&) const;
	virtual void		getCursorPos(SInt32&, SInt32&) const;

	// IPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual UInt32		addOneShotTimer(double timeout);
	virtual SInt32		getJumpZoneSize() const;
	virtual bool		isAnyMouseButtonDown() const;
	virtual const char*	getKeyName(KeyButton) const;

	// ISecondaryScreen overrides
	virtual void		fakeKeyEvent(KeyButton id, bool press) const;
	virtual bool		fakeCtrlAltDel() const;
	virtual void		fakeMouseButton(ButtonID id, bool press) const;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const;
	virtual void		fakeMouseWheel(SInt32 delta) const;
	virtual KeyButton	mapKey(IKeyState::Keystrokes&,
							const IKeyState& keyState, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const;

private:
	// process events before dispatching to receiver
	void				onEvent(XEvent* event);

	// create the transparent cursor
	Cursor				createBlankCursor() const;

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
	class CKeyEventInfo {
	public:
		int				m_event;
		Window			m_window;
		Time			m_time;
		KeyCode			m_keycode;
	};

	bool				isQuitEvent(XEvent*) const;

	Window				createWindow() const;
	void				openIM();

	bool				grabMouseAndKeyboard();
	void				onKeyPress(XKeyEvent&);
	void				onKeyRelease(XKeyEvent&);
	void				onMousePress(const XButtonEvent&);
	void				onMouseRelease(const XButtonEvent&);
	void				onMouseMove(const XMotionEvent&);

	void				selectEvents(Window) const;
	void				doSelectEvents(Window) const;

	KeyID				mapKeyFromX(XKeyEvent*) const;
	ButtonID			mapButtonFromX(const XButtonEvent*) const;
	unsigned int		mapButtonToX(ButtonID id) const;

	void				warpCursorNoFlush(SInt32 x, SInt32 y);

	void				updateButtons();

	static Bool			findKeyEvent(Display*, XEvent* xevent, XPointer arg);

private:
	typedef CPriorityQueue<CTimer> CTimerPriorityQueue;

	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;

	// X is not thread safe
	CMutex				m_mutex;

	Display*			m_display;
	Window				m_root;
	Window				m_window;

	IScreenReceiver*		m_receiver;
	IPrimaryScreenReceiver*	m_primaryReceiver;

	// true if mouse has entered the screen
	bool				m_isOnScreen;

	// screen shape stuff
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// last mouse position
	SInt32				m_xCursor, m_yCursor;

	// keyboard stuff
	IKeyState*			m_keyState;
	CXWindowsKeyMapper	m_keyMapper;

	// input method stuff
	XIM					m_im;
	XIC					m_ic;
	KeyCode				m_lastKeycode;

	// clipboards
	CXWindowsClipboard*	m_clipboard[kClipboardEnd];

	// the quit message
	Atom				m_atomQuit;

	// screen saver stuff
	CXWindowsScreenSaver*	m_screensaver;
	bool				m_screensaverNotify;
	Atom				m_atomScreensaver;

	// timers, the stopwatch used to time, and a mutex for the timers
	CTimerPriorityQueue	m_timers;
	CStopwatch			m_time;
	CMutex				m_timersMutex;
	CTimer*				m_oneShotTimer;

	// logical to physical button mapping.  m_buttons[i] gives the
	// physical button for logical button i+1.
	std::vector<unsigned char>	m_buttons;

	// true if global auto-repeat was enabled before we turned it off
	bool				m_autoRepeat;

	// stuff to workaround xtest being xinerama unaware.  attempting
	// to fake a mouse motion under xinerama may behave strangely,
	// especially if screen 0 is not at 0,0 or if faking a motion on
	// a screen other than screen 0.
	bool				m_xtestIsXineramaUnaware;
	bool				m_xinerama;

	// pointer to (singleton) screen.  this is only needed by
	// ioErrorHandler().
	static CXWindowsScreen*	s_screen;
};

#endif
