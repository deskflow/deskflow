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

#ifndef CXWINDOWSPRIMARYSCREEN_H
#define CXWINDOWSPRIMARYSCREEN_H

#include "CPrimaryScreen.h"
#include "IScreenEventHandler.h"
#include "MouseTypes.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class CXWindowsScreen;
class IScreenReceiver;
class IPrimaryScreenReceiver;

//! X11 primary screen implementation
class CXWindowsPrimaryScreen :
				public CPrimaryScreen, public IScreenEventHandler {
public:
	CXWindowsPrimaryScreen(IScreenReceiver*, IPrimaryScreenReceiver*);
	virtual ~CXWindowsPrimaryScreen();

	// CPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual UInt32		addOneShotTimer(double timeout);
	virtual KeyModifierMask	getToggleMask() const;
	virtual bool		isLockedToScreen() const;
	virtual IScreen*	getScreen() const;

	// IScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onOneShotTimerExpired(UInt32 id);
	virtual SInt32		getJumpZoneSize() const;

protected:
	// CPrimaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPreClose();
	virtual void		onPreEnter();
	virtual void		onPreLeave();
	virtual void		onEnterScreenSaver();

	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual bool		showWindow();
	virtual void		hideWindow();
	virtual void		warpCursorToCenter();

	virtual void		updateKeys();

private:
	void				warpCursorNoFlush(Display*,
							SInt32 xAbsolute, SInt32 yAbsolute);

	void				selectEvents(Display*, Window) const;
	void				doSelectEvents(Display*, Window) const;

	KeyModifierMask		mapModifier(unsigned int state) const;
	KeyID				mapKey(XKeyEvent*) const;
	ButtonID			mapButton(unsigned int button) const;

	class CKeyEventInfo {
	public:
		int				m_event;
		Window			m_window;
		Time			m_time;
		KeyCode			m_keycode;
	};
	static Bool			findKeyEvent(Display*, XEvent* xevent, XPointer arg);

private:
	CXWindowsScreen*	m_screen;
	IPrimaryScreenReceiver*	m_receiver;

	// our window
	Window				m_window;

	// note toggle keys that toggle on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;

	// modifier masks
	unsigned int		m_altMask;
	unsigned int		m_metaMask;
	unsigned int		m_superMask;
	unsigned int		m_modeSwitchMask;
	unsigned int		m_numLockMask;
	unsigned int		m_capsLockMask;
	unsigned int		m_scrollLockMask;

	// last mouse position
	SInt32				m_x, m_y;

	// position of center pixel of screen
	SInt32				m_xCenter, m_yCenter;

	// input method stuff
	XIM					m_im;
	XIC					m_ic;
	KeyCode				m_lastKeycode;
};

#endif
