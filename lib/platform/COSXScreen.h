/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#ifndef COSXSCREEN_H
#define COSXSCREEN_H

#include "CPlatformScreen.h"
#include "stdvector.h"
#include <Carbon/Carbon.h>

class COSXKeyState;
class COSXScreenSaver;

//! Implementation of IPlatformScreen for OS X
class COSXScreen : public CPlatformScreen {
public:
	COSXScreen(bool isPrimary);
	virtual ~COSXScreen();

	//! @name manipulators
	//@{

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
	void				updateScreenShape();

private:
	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;

	// true if mouse has entered the screen
	bool				m_isOnScreen;

	// the display
	CGDirectDisplayID	m_displayID;

	// screen shape stuff
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// mouse state
	mutable SInt32		m_xCursor, m_yCursor;
	mutable boolean_t	m_buttons[5];
	bool				m_cursorHidden;

	// keyboard stuff
	COSXKeyState*		m_keyState;

	// clipboards
	UInt32				m_sequenceNumber;

	// screen saver stuff
	COSXScreenSaver*	m_screensaver;
	bool				m_screensaverNotify;
};

#endif
