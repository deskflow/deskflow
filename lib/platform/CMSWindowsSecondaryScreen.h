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

#ifndef CMSWINDOWSSECONDARYSCREEN_H
#define CMSWINDOWSSECONDARYSCREEN_H

// ensure that we get SendInput()
#if _WIN32_WINNT <= 0x400
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x401
#endif

#include "CSecondaryScreen.h"
#include "IMSWindowsScreenEventHandler.h"
#include "CMutex.h"
#include "CString.h"
#include "stdvector.h"

class CMSWindowsScreen;
class IScreenReceiver;

//! Microsoft windows secondary screen implementation
class CMSWindowsSecondaryScreen :
				public CSecondaryScreen, public IMSWindowsScreenEventHandler {
public:
	CMSWindowsSecondaryScreen(IScreenReceiver*);
	virtual ~CMSWindowsSecondaryScreen();

	// CSecondaryScreen overrides
	virtual void		keyDown(KeyID, KeyModifierMask);
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		keyUp(KeyID, KeyModifierMask);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual IScreen*	getScreen() const;

	// IMSWindowsScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onOneShotTimerExpired(UInt32 id);
	virtual SInt32		getJumpZoneSize() const;
	virtual void		postCreateWindow(HWND);
	virtual void		preDestroyWindow(HWND);

protected:
	// CSecondaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPreEnter();
	virtual void		onPreLeave();
	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual void		showWindow();
	virtual void		hideWindow();
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual void		updateKeys();
	virtual void		releaseKeys();
	virtual void		setToggleState(KeyModifierMask);
	virtual KeyModifierMask	getToggleState() const;

private:
	enum EKeyAction { kPress, kRelease, kRepeat };
	class Keystroke {
	public:
		UINT			m_virtualKey;
		bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;

	// open/close desktop (for windows 95/98/me)
	bool				openDesktop();
	void				closeDesktop();

	// make desk the thread desktop (for windows NT/2000/XP)
	bool				switchDesktop(HDESK desk);

	// returns true iff there appear to be multiple monitors
	bool				isMultimon() const;

	// key and button queries and operations
	DWORD				mapButton(ButtonID button, bool press) const;
	KeyModifierMask		mapKey(Keystrokes&, UINT& virtualKey, KeyID,
							KeyModifierMask, EKeyAction) const;
	void				doKeystrokes(const Keystrokes&, SInt32 count);

	void				toggleKey(UINT virtualKey, KeyModifierMask mask);
	UINT				virtualKeyToScanCode(UINT& virtualKey) const;
	bool				isExtendedKey(UINT virtualKey) const;
	void				sendKeyEvent(UINT virtualKey, bool press);

private:
	CMutex				m_mutex;
	CMSWindowsScreen*	m_screen;

	// true if windows 95/98/me
	bool				m_is95Family;

	// our window
	HWND				m_window;

	// virtual key states as set by us or the user
	BYTE				m_keys[256];

	// virtual key states as set by us
	BYTE				m_fakeKeys[256];

	// current active modifiers
	KeyModifierMask		m_mask;
};

#endif
