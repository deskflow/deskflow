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

#ifndef CMSWINDOWSSCREEN_H
#define CMSWINDOWSSCREEN_H

#include "IPlatformScreen.h"
#include "CMSWindowsKeyMapper.h"
#include "CSynergyHook.h"
#include "CMutex.h"
#include "CString.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsScreenSaver;
class IScreenReceiver;
class IPrimaryScreenReceiver;

//! Implementation of IPlatformScreen for Microsoft Windows
class CMSWindowsScreen : public IPlatformScreen {
public:
	CMSWindowsScreen(IScreenReceiver*, IPrimaryScreenReceiver*);
	virtual ~CMSWindowsScreen();

	//! @name manipulators
	//@{

	//! Initialize
	/*!
	Saves the application's HINSTANCE.  This \b must be called by
	WinMain with the HINSTANCE it was passed.
	*/
	static void			init(HINSTANCE);

	//@}
	//! @name accessors
	//@{

	//! Get instance
	/*!
	Returns the application instance handle passed to init().
	*/
	static HINSTANCE	getInstance();

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
	// update screen size cache
	void				updateScreenShape();

	// switch to the given desktop.  this destroys the window and unhooks
	// all hooks, switches the desktop, then creates the window and rehooks
	// all hooks (because you can't switch the thread's desktop if it has
	// any windows or hooks).
	bool				switchDesktop(HDESK desk);

	// make sure we're on the expected desktop
	void				syncDesktop() const;

	// handle message before it gets dispatched.  returns true iff
	// the message should not be dispatched.
	bool				onPreDispatch(HWND, UINT, WPARAM, LPARAM);

	// handle message before it gets dispatched.  returns true iff
	// the message should not be dispatched.
	bool				onPreDispatchPrimary(HWND, UINT, WPARAM, LPARAM);

	// handle message.  returns true iff handled and optionally sets
	// \c *result (which defaults to 0).
	bool				onEvent(HWND, UINT, WPARAM, LPARAM, LRESULT* result);

	// message handlers
	bool				onMark(UInt32 mark);
	bool				onKey(WPARAM, LPARAM);
	bool				onMouseButton(WPARAM, LPARAM);
	bool				onMouseMove(SInt32 x, SInt32 y);
	bool				onMouseWheel(SInt32 delta);
	bool				onScreensaver(bool activated);
	bool				onTimer(UINT timerID);
	bool				onDisplayChange();
	bool				onClipboardChange();
	bool				onActivate(bool activated);

// XXX
	// warp cursor without discarding queued events
	void				warpCursorNoFlush(SInt32 x, SInt32 y);

	// discard posted messages
	void				nextMark();

	// test if event should be ignored
	bool				ignore() const;
// XXX

	// create the transparent cursor
	HCURSOR				createBlankCursor() const;

	// show/hide the cursor
	void				showCursor(bool) const;

	// enable/disable special key combinations so we can catch/pass them
	void				enableSpecialKeys(bool) const;

	// map a button ID and action to a mouse event
	DWORD				mapButtonToEvent(ButtonID button,
							bool press, DWORD* inData) const;

	// map a button event to a button ID
	ButtonID			mapButtonFromEvent(WPARAM msg, LPARAM button) const;

	// return true iff the given virtual key is a modifier
	bool				isModifier(UINT vkCode) const;

	// send ctrl+alt+del hotkey event
	static void			ctrlAltDelThread(void*);

	// our window proc
	static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

private:
	static HINSTANCE	s_instance;

	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;

	// true if windows 95/98/me
	bool				m_is95Family;

	// receivers
	IScreenReceiver*		m_receiver;
	IPrimaryScreenReceiver*	m_primaryReceiver;

	// true if mouse has entered the screen
	bool				m_isOnScreen;

	// our resources
	ATOM				m_class;
	HCURSOR				m_cursor;
	HWND				m_window;

	// screen shape stuff
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// true if system appears to have multiple monitors
	bool				m_multimon;

	// last mouse position
	SInt32				m_xCursor, m_yCursor;

	// used to discard queued messages that are no longer needed
	UInt32				m_mark;
	UInt32				m_markReceived;

	// the main loop's thread id
	DWORD				m_threadID;

	// the thread id of the last attached thread
	DWORD				m_lastThreadID;

	// the timer used to check for desktop switching
	UINT				m_timer;

	// the one shot timer
	UINT				m_oneShotTimer;

	// screen saver stuff
	CMSWindowsScreenSaver*	m_screensaver;
	bool					m_screensaverNotify;

	// clipboard stuff
	HWND				m_nextClipboardWindow;
	bool				m_ownClipboard;

	// the current desk and it's name
	HDESK				m_desk;
	CString				m_deskName;

	// true when the current desktop is inaccessible.  while
	// the desktop is inaccessible we won't receive user input
	// and we'll lose track of the keyboard state.  when the
	// desktop becomes accessible again we'll notify the event
	// handler of that.
	bool				m_inaccessibleDesktop;

	// hook library stuff
	HINSTANCE			m_hookLibrary;
	InitFunc			m_init;
	CleanupFunc			m_cleanup;
	InstallFunc			m_install;
	UninstallFunc		m_uninstall;
	SetSidesFunc		m_setSides;
	SetZoneFunc			m_setZone;
	SetModeFunc			m_setMode;
	InstallScreenSaverFunc		m_installScreensaver;
	UninstallScreenSaverFunc	m_uninstallScreensaver;
	bool				m_lowLevel;

	// keyboard stuff
	IKeyState*			m_keyState;
	CMSWindowsKeyMapper	m_keyMapper;

	// map of button state
	BYTE				m_buttons[1 + kButtonExtra0 + 1];

	// stuff for hiding the cursor
	DWORD				m_cursorThread;

	static CMSWindowsScreen*	s_screen;
};

#endif
