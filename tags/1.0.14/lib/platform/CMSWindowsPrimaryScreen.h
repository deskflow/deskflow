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

#ifndef CMSWINDOWSPRIMARYSCREEN_H
#define CMSWINDOWSPRIMARYSCREEN_H

#include "CPrimaryScreen.h"
#include "IMSWindowsScreenEventHandler.h"
#include "CSynergyHook.h"
#include "MouseTypes.h"
#include "CString.h"

class CMSWindowsScreen;
class IScreenReceiver;
class IPrimaryScreenReceiver;

//! Microsoft windows primary screen implementation
class CMSWindowsPrimaryScreen :
				public CPrimaryScreen, public IMSWindowsScreenEventHandler {
public:
	typedef bool (CMSWindowsPrimaryScreen::*HookMethod)(int, WPARAM, LPARAM);

	CMSWindowsPrimaryScreen(IScreenReceiver*, IPrimaryScreenReceiver*);
	virtual ~CMSWindowsPrimaryScreen();

	// CPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual UInt32		addOneShotTimer(double timeout);
	virtual KeyModifierMask	getToggleMask() const;
	virtual bool		isLockedToScreen() const;
	virtual IScreen*	getScreen() const;

	// IMSWindowsScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onOneShotTimerExpired(UInt32 id);
	virtual SInt32		getJumpZoneSize() const;
	virtual void		postCreateWindow(HWND);
	virtual void		preDestroyWindow(HWND);
	virtual void		onAccessibleDesktop();

protected:
	// CPrimaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPostClose();
	virtual void		onPreEnter();
	virtual void		onPostEnter();
	virtual void		onPreLeave();
	virtual void		onPostLeave(bool);

	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual bool		showWindow();
	virtual void		hideWindow();
	virtual void		warpCursorToCenter();

	virtual void		updateKeys();

private:
	void				enterNoWarp();

	// warp cursor without discarding queued events
	void				warpCursorNoFlush(SInt32 x, SInt32 y);

	// discard posted messages
	void				nextMark();

	// test if event should be ignored
	bool				ignore() const;

	// key and button queries
	KeyID				mapKey(WPARAM keycode, LPARAM info,
							KeyModifierMask* maskOut, bool* altgr);
	ButtonID			mapButton(WPARAM msg, LPARAM button) const;
	void				updateKey(UINT vkCode, bool press);
	bool				isModifier(UINT vkCode) const;
	KeyButton			mapKeyToScanCode(UINT vk1, UINT vk2) const;

private:
	IPrimaryScreenReceiver*	m_receiver;
	CMSWindowsScreen*	m_screen;

	// true if windows 95/98/me
	bool				m_is95Family;

	// the main loop's thread id
	DWORD				m_threadID;

	// my window
	HWND				m_window;

	// used to discard queued messages that are no longer needed
	UInt32				m_mark;
	UInt32				m_markReceived;

	// map of key state
	BYTE				m_keys[256];

	// map of button state
	BYTE				m_buttons[1 + kButtonExtra0 + 1];

	// last mouse position
	SInt32				m_x, m_y;

	// position of center pixel of screen
	SInt32				m_xCenter, m_yCenter;

	// hook library stuff
	HINSTANCE			m_hookLibrary;
	InitFunc			m_init;
	CleanupFunc			m_cleanup;
	InstallFunc			m_install;
	UninstallFunc		m_uninstall;
	SetSidesFunc		m_setSides;
	SetZoneFunc			m_setZone;
	SetRelayFunc		m_setRelay;
	bool				m_lowLevel;

	// stuff for hiding the cursor
	DWORD				m_cursorThread;
};

#endif
