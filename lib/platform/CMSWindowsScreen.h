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
#include "CCondVar.h"
#include "CMutex.h"
#include "CString.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CEventQueueTimer;
class CMSWindowsScreenSaver;
class CThread;
class IJob;

//! Implementation of IPlatformScreen for Microsoft Windows
class CMSWindowsScreen : public IPlatformScreen {
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

	// IPlatformScreen overrides
	virtual void		setKeyState(IKeyState*);
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
	virtual void		updateKeys();
	virtual void		setSequenceNumber(UInt32);
	virtual bool		isPrimary() const;

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
	class CDesk {
	public:
		CString			m_name;
		CThread*		m_thread;
		DWORD			m_threadID;
		DWORD			m_targetID;
		HDESK			m_desk;
		HWND			m_window;
		bool			m_lowLevel;
	};
	typedef std::map<CString, CDesk*> CDesks;

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

	// system event handler (does DispatchMessage)
	void				handleSystemEvent(const CEvent& event, void*);

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

	// our desk window procs
	static LRESULT CALLBACK primaryDeskProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK secondaryDeskProc(HWND, UINT, WPARAM, LPARAM);

	void				deskMouseMove(SInt32 x, SInt32 y) const;
	void				deskEnter(CDesk* desk);
	void				deskLeave(CDesk* desk, HKL keyLayout);
	void				deskThread(void* vdesk);
	CDesk*				addDesk(const CString& name, HDESK hdesk);
	void				removeDesks();
	void				checkDesk();
	bool				isDeskAccessible(const CDesk* desk) const;
	void				sendDeskMessage(UINT, WPARAM, LPARAM) const;
	void				waitForDesk() const;
	void				handleCheckDesk(const CEvent& event, void*);

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
	ATOM				m_deskClass;
	HCURSOR				m_cursor;

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

	// the timer used to check for desktop switching
	CEventQueueTimer*	m_timer;

	// screen saver stuff
	CMSWindowsScreenSaver*	m_screensaver;
	bool					m_screensaverNotify;

	// clipboard stuff.  our window is used mainly as a clipboard
	// owner and as a link in the clipboard viewer chain.
	HWND				m_window;
	HWND				m_nextClipboardWindow;
	bool				m_ownClipboard;

	// the current desk and it's name
	CDesk*				m_activeDesk;
	CString				m_activeDeskName;

	// one desk per desktop and a cond var to communicate with it
	CMutex				m_mutex;
	CCondVar<bool>		m_deskReady;
	CDesks				m_desks;

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

	// keyboard stuff
	IKeyState*			m_keyState;
	CMSWindowsKeyMapper	m_keyMapper;

	// map of button state
	BYTE				m_buttons[1 + kButtonExtra0 + 1];

	// suspend/resume callbacks
	IJob*				m_suspend;
	IJob*				m_resume;

	static CMSWindowsScreen*	s_screen;
};

#endif
