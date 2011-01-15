/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CPLATFORMSCREEN_H
#define CPLATFORMSCREEN_H

#include "IPlatformScreen.h"

//! Base screen implementation
/*!
This screen implementation is the superclass of all other screen
implementations.  It implements a handful of methods and requires
subclasses to implement the rest.
*/
class CPlatformScreen : public IPlatformScreen {
public:
	CPlatformScreen();
	virtual ~CPlatformScreen();
	
	static IPlatformScreen* getInstance();

	// IScreen overrides
	virtual void*		getEventTarget() const = 0;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const = 0;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;
	// IPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides) = 0;
	virtual void		warpCursor(SInt32 x, SInt32 y, UInt8 id) = 0;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const = 0;
	virtual UInt32		registerHotKey(KeyID key, KeyModifierMask mask, UInt8 id) = 0;
	virtual void		unregisterHotKey(UInt32 id) = 0;
	virtual void		fakeInputBegin() = 0;
	virtual void		fakeInputEnd() = 0;
	virtual SInt32		getJumpZoneSize() const = 0;
	virtual bool		isAnyMouseButtonDown(UInt8 id) const = 0;

	// ISecondaryScreen overrides
	virtual void		fakeMouseButton(ButtonID id, bool press) const = 0;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const = 0;
	virtual void		fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const = 0;
	virtual void		fakeMouseWheel(SInt32 xDelta, SInt32 yDelta) const = 0;

	// MPX fake methods:
	virtual void            fakeMotionEvent(SInt32 x, SInt32 y, UInt8 id) const = 0;
	virtual void            fakeRelativeMotionEvent(SInt32 x, SInt32 y, UInt8 id) const = 0;
	virtual void            fakeButtonEvent(ButtonID button, bool press, UInt8 id) const = 0;
	virtual void		fakeMouseWheelEvent(SInt32 xDelta, SInt32 yDelta, UInt8 id) const = 0;
	
	// IKeyState overrides
	virtual void		updateKeyMap(UInt8 id);
	virtual void		updateKeyState(UInt8 id);
	virtual void		setHalfDuplexMask(KeyModifierMask, UInt8 id);
	virtual void		fakeKeyDown(KeyID kId, KeyModifierMask mask,
							KeyButton button, UInt8 id);
	virtual void		fakeKeyRepeat(KeyID kId, KeyModifierMask mask,
							SInt32 count, KeyButton button, UInt8 id);
	virtual void		fakeKeyUp(KeyButton button, UInt8 id);
	virtual void		fakeAllKeysUp(UInt8 id);
	virtual bool		fakeCtrlAltDel(UInt8 id);
	virtual bool		isKeyDown(KeyButton, UInt8 id) const;
	virtual KeyModifierMask
						getActiveModifiers(UInt8 id) const;
	virtual KeyModifierMask
						pollActiveModifiers(UInt8 id) const;
	virtual SInt32		pollActiveGroup(UInt8 id) const;
	virtual void		pollPressedKeys(IKeyState::KeyButtonSet& pressedKeys, UInt8 id) const;

	// IPlatformScreen overrides
	virtual void		enable() = 0;
	virtual void		disable() = 0;
	virtual void		enter(UInt8 kId, UInt8 pId) = 0;
	virtual bool		leave(UInt8 id) = 0;
	virtual bool		setClipboard(ClipboardID, const IClipboard*) = 0;
	virtual void		checkClipboards() = 0;
	virtual void		openScreensaver(bool notify) = 0;
	virtual void		closeScreensaver() = 0;
	virtual void		screensaver(bool activate) = 0;
	virtual void		resetOptions() = 0;
	virtual void		setOptions(const COptionsList& options) = 0;
	virtual void		setSequenceNumber(UInt32) = 0;
	virtual bool		isPrimary() const = 0;

protected:
	//! Update mouse buttons
	/*!
	Subclasses must implement this method to update their internal mouse
	button mapping and, if desired, state tracking.
	*/
	virtual void		updateButtons() = 0;

	//! Get the key state
	/*!
	Subclasses must implement this method to return the platform specific
	key state object that each subclass must have.
	*/
	virtual IKeyState*	getKeyState(UInt8 id) const = 0;

	// IPlatformScreen overrides
	virtual void		handleSystemEvent(const CEvent& event, void*) = 0;
};

#endif
