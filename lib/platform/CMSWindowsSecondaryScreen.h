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

class CMSWindowsScreen;
class IScreenReceiver;

//! Microsoft windows secondary screen implementation
class CMSWindowsSecondaryScreen :
				public CSecondaryScreen, public IMSWindowsScreenEventHandler {
public:
	CMSWindowsSecondaryScreen(IScreenReceiver*);
	virtual ~CMSWindowsSecondaryScreen();

	// CSecondaryScreen overrides
	virtual IScreen*	getScreen() const;

	// IMSWindowsScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onOneShotTimerExpired(UInt32 id);
	virtual void		postCreateWindow(HWND);
	virtual void		preDestroyWindow(HWND);
	virtual void		onAccessibleDesktop();

protected:
	// CSecondaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPreEnter();
	virtual void		onPreLeave();
	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual void		showWindow(SInt32 x, SInt32 y);
	virtual void		hideWindow();
	virtual void		updateKeys(KeyState* sysKeyStates);
	virtual KeyModifierMask	getModifiers() const;

	virtual SysKeyID	getUnhanded(SysKeyID) const;
	virtual SysKeyID	getOtherHanded(SysKeyID) const;
	virtual bool		isAutoRepeating(SysKeyID) const;
	virtual KeyModifierMask	getModifierKeyMask(SysKeyID) const;
	virtual bool		isModifierActive(SysKeyID) const;
	virtual SysKeyID	getToggleSysKey(KeyID keyID) const;
	virtual bool		synthesizeCtrlAltDel(EKeyAction);
	virtual void		sync() const;
	virtual KeyModifierMask
						mapKey(Keystrokes&, SysKeyID& sysKeyID, KeyID,
							KeyModifierMask, KeyModifierMask, EKeyAction) const;
	virtual void		fakeKeyEvent(SysKeyID, bool press) const;
	virtual void		fakeMouseButton(ButtonID, bool press) const;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const;
	virtual void		fakeMouseWheel(SInt32 delta) const;

private:
	class CModifierInfo {
	public:
		KeyModifierMask	m_mask;
		UINT			m_virtualKey;
		UINT			m_virtualKey2;
		bool			m_isToggle;
	};

	// open/close desktop (for windows 95/98/me)
	bool				openDesktop();
	void				closeDesktop();

	// make desk the thread desktop (for windows NT/2000/XP)
	bool				switchDesktop(HDESK desk);

	// returns true iff there appear to be multiple monitors
	bool				isMultimon() const;

	// key and button queries and operations
	DWORD				mapButton(ButtonID button,
							bool press, DWORD* data) const;
	UINT				mapCharacter(Keystrokes& keys,
							char c, HKL hkl,
							KeyModifierMask currentMask,
							KeyModifierMask desiredMask,
							EKeyAction action) const;
	KeyModifierMask		mapToKeystrokes(Keystrokes& keys,
							UINT virtualKey,
							KeyModifierMask currentMask,
							KeyModifierMask desiredMask,
							EKeyAction action) const;
	const CModifierInfo*	getModifierInfo(UINT virtualKey) const;

	KeyState			getKeyState(UINT virtualKey) const;
	UINT				virtualKeyToScanCode(UINT& virtualKey) const;
	bool				isExtendedKey(UINT virtualKey) const;

	UINT				getCodePageFromLangID(LANGID) const;

	// thread that generates fake ctrl+alt+del
	static void			ctrlAltDelThread(void*);

private:
	CMutex				m_mutex;
	CMSWindowsScreen*	m_screen;

	// true if windows 95/98/me
	bool				m_is95Family;

	// our window
	HWND				m_window;

	// modifier table
	static const CModifierInfo	s_modifier[];
};

#endif
