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

#include "CPlatformScreen.h"
#include "CSynergyHook.h"
#include "CCondVar.h"
#include "CMutex.h"
#include "CString.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsDesks;
class CMSWindowsKeyState;
class CMSWindowsScreenSaver;
class CThread;
class IJob;

//! Implementation of IPlatformScreen for Microsoft Windows
class CMSWindowsScreen : public CPlatformScreen {
public:
	CMSWindowsScreen(bool isPrimary, IJob* suspend, IJob* resume);
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

	// IScreen overrides
	virtual void*		getEventTarget() const;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	// IPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual SInt32		getJumpZoneSize() const;
	virtual bool		isAnyMouseButtonDown() const;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const;

	// ISecondaryScreen overrides
	virtual void		fakeMouseButton(ButtonID id, bool press) const;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const;
	virtual void		fakeMouseWheel(SInt32 delta) const;

	// IKeyState overrides
	virtual void		updateKeys();

	// IPlatformScreen overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		enter();
	virtual bool		leave();
	virtual bool		setClipboard(ClipboardID, const IClipboard*);
	virtual void		checkClipboards();
	virtual void		openScreensaver(bool notify);
	virtual void		closeScreensaver();
	virtual void		screensaver(bool activate);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual void		setSequenceNumber(UInt32);
	virtual bool		isPrimary() const;

protected:
	// IPlatformScreen overrides
	virtual void		handleSystemEvent(const CEvent&, void*);
	virtual void		updateButtons();
	virtual IKeyState*	getKeyState() const;

private:
	// initialization and shutdown operations
	HINSTANCE			openHookLibrary(const char* name);
	void				closeHookLibrary(HINSTANCE hookLibrary) const;
	HCURSOR				createBlankCursor() const;
	void				destroyCursor(HCURSOR cursor) const;
	ATOM				createWindowClass() const;
	ATOM				createDeskWindowClass(bool isPrimary) const;
	void				destroyClass(ATOM windowClass) const;
	HWND				createWindow(ATOM windowClass, const char* name) const;
	void				destroyWindow(HWND) const;

	// convenience function to send events
	void				sendEvent(CEvent::Type type, void* = NULL);
	void				sendClipboardEvent(CEvent::Type type, ClipboardID id);

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
	bool				onDisplayChange();
	bool				onClipboardChange();

	// warp cursor without discarding queued events
	void				warpCursorNoFlush(SInt32 x, SInt32 y);

	// discard posted messages
	void				nextMark();

	// test if event should be ignored
	bool				ignore() const;

	// update screen size cache
	void				updateScreenShape();

	// enable/disable special key combinations so we can catch/pass them
	void				enableSpecialKeys(bool) const;

	// map a button event to a button ID
	ButtonID			mapButtonFromEvent(WPARAM msg, LPARAM button) const;

	// job to update the key state
	void				updateKeysCB(void*);

	// our window proc
	static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

private:
	static HINSTANCE	s_instance;

	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;

	// true if windows 95/98/me
	bool				m_is95Family;

	// true if mouse has entered the screen
	bool				m_isOnScreen;

	// our resources
	ATOM				m_class;

	// screen shape stuff
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// true if system appears to have multiple monitors
	bool				m_multimon;

	// last mouse position
	SInt32				m_xCursor, m_yCursor;

	UInt32				m_sequenceNumber;

	// used to discard queued messages that are no longer needed
	UInt32				m_mark;
	UInt32				m_markReceived;

	// the main loop's thread id
	DWORD				m_threadID;

	// the keyboard layout to use when off primary screen
	HKL					m_keyLayout;

	// screen saver stuff
	CMSWindowsScreenSaver*	m_screensaver;
	bool					m_screensaverNotify;

	// clipboard stuff.  our window is used mainly as a clipboard
	// owner and as a link in the clipboard viewer chain.
	HWND				m_window;
	HWND				m_nextClipboardWindow;
	bool				m_ownClipboard;

	// one desk per desktop and a cond var to communicate with it
	CMSWindowsDesks*	m_desks;

	// hook library stuff
	HINSTANCE			m_hookLibrary;
	InitFunc			m_init;
	CleanupFunc			m_cleanup;
	SetSidesFunc		m_setSides;
	SetZoneFunc			m_setZone;
	SetModeFunc			m_setMode;

	// keyboard stuff
	CMSWindowsKeyState*	m_keyState;

	// map of button state
	bool				m_buttons[1 + kButtonExtra0 + 1];

	// suspend/resume callbacks
	IJob*				m_suspend;
	IJob*				m_resume;

	static CMSWindowsScreen*	s_screen;
};

#endif
