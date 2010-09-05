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
#include "stdmap.h"
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
	virtual void		keyDown(KeyID, KeyModifierMask, KeyButton);
	virtual void		keyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton);
	virtual void		keyUp(KeyID, KeyModifierMask, KeyButton);
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
	class CModifierInfo {
	public:
		KeyModifierMask	m_mask;
		UINT			m_virtualKey;
		UINT			m_virtualKey2;
		bool			m_isToggle;
	};
	typedef std::vector<Keystroke> Keystrokes;
	typedef std::map<KeyButton, UINT> ServerKeyMap;

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
	KeyModifierMask		mapKey(Keystrokes&, UINT& virtualKey, KeyID,
							KeyModifierMask, EKeyAction) const;
	KeyModifierMask		mapKeyRelease(Keystrokes& keys, UINT virtualKey) const;
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
	void				doKeystrokes(const Keystrokes&, SInt32 count);
	const CModifierInfo*	getModifierInfo(UINT virtualKey) const;

	void				toggleKey(UINT virtualKey, KeyModifierMask mask);
	UINT				virtualKeyToScanCode(UINT& virtualKey) const;
	bool				isExtendedKey(UINT virtualKey) const;
	void				sendKeyEvent(UINT virtualKey, bool press);

	UINT				getCodePageFromLangID(LANGID) const;

	// generate a fake ctrl+alt+del
	void				synthesizeCtrlAltDel();

	// thread that generates fake ctrl+alt+del
	static void			ctrlAltDelThread(void*);

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

	// map server key buttons to local virtual keys
	ServerKeyMap		m_serverKeyMap;

	// modifier table
	static const CModifierInfo	s_modifier[];
};

#endif
