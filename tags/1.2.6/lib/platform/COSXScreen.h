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

#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

template <class T>
class CCondVar;
class CMutex;
class CThread;
class COSXKeyState;
class COSXScreenSaver;

//! Implementation of IPlatformScreen for OS X
class COSXScreen : public CPlatformScreen {
public:
	COSXScreen(bool isPrimary);
	virtual ~COSXScreen();

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
	virtual void		fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const;
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
	void				postMouseEvent(const CGPoint &) const;
	
	// convenience function to send events
	void				sendEvent(CEvent::Type type, void* = NULL) const;
	void				sendClipboardEvent(CEvent::Type type, ClipboardID id) const;

	// message handlers
	bool				onMouseMove(SInt32 x, SInt32 y);
	// mouse button handler.  pressed is true if this is a mousedown
	// event, false if it is a mouseup event.  macButton is the index
	// of the button pressed using the mac button mapping.
	bool				onMouseButton(bool pressed, UInt16 macButton) const;
	bool				onMouseWheel(SInt32 delta) const;

	bool				onDisplayChange();

	bool				onKey(EventRef event) const;

	// map mac mouse button to synergy buttons
	ButtonID			mapMacButtonToSynergy(UInt16) const;

	// map mac scroll wheel value to a synergy scroll wheel value
	SInt32				mapScrollWheelToSynergy(SInt32) const;

	// map synergy scroll wheel value to a mac scroll wheel value
	SInt32				mapScrollWheelFromSynergy(SInt32) const;

	// get the current scroll wheel speed
	double				getScrollSpeed() const;

	// get the current scroll wheel speed
	double				getScrollSpeedFactor() const;

	// map mac modifier mask to synergy modifier mask
	KeyModifierMask		mapMacModifiersToSynergy(EventRef event) const;

	// Resolution switch callback
	static pascal void	displayManagerCallback(void* inUserData,
							SInt16 inMessage, void* inNotifyData);
	
	// fast user switch callback
	static pascal OSStatus userSwitchCallback (EventHandlerCallRef nextHandler,
											   EventRef theEvent,
											   void* inUserData);
	
	// sleep / wakeup support
	void watchSystemPowerThread(void*);
	static void testCanceled (CFRunLoopTimerRef timer, void *info);


	static void powerChangeCallback(void * refcon, io_service_t service,
								natural_t messageType, void * messageArgument);
	
	void		handlePowerChangeRequest(natural_t messageType,
										 void * messageArgument);

	static CEvent::Type		getConfirmSleepEvent();
	void		handleConfirmSleep(const CEvent& event, void*);
	
	// global hotkey operating mode
	static bool isGlobalHotKeyOperatingModeAvailable();
	static void setGlobalHotKeysEnabled(bool enabled);
	static bool getGlobalHotKeysEnabled();

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
	mutable bool		m_cursorPosValid;
	mutable boolean_t	m_buttons[5];
	bool				m_cursorHidden;

	// keyboard stuff
	COSXKeyState*		m_keyState;

	// clipboards
	UInt32				m_sequenceNumber;

	// screen saver stuff
	COSXScreenSaver*	m_screensaver;
	bool				m_screensaverNotify;

	// clipboard stuff
	bool				m_ownClipboard;

	// window object that gets user input events when the server
	// has focus.
	WindowRef			m_hiddenWindow;
	// window object that gets user input events when the server
	// does not have focus.
	WindowRef			m_userInputWindow;

	// display manager stuff (to get screen resolution switches).
	DMExtendedNotificationUPP   m_displayManagerNotificationUPP;
	ProcessSerialNumber			m_PSN;

	// fast user switching
	EventHandlerRef			m_switchEventHandlerRef;

	// sleep / wakeup
	CMutex*					m_pmMutex;
	CThread*				m_pmWatchThread;
    CCondVar<bool>*			m_pmThreadReady;
	CFRunLoopRef			m_pmRunloop;
	io_connect_t			m_pmRootPort;

	// global hotkey operating mode
	static bool				s_testedForGHOM;
	static bool				s_hasGHOM;

	// events
	static CEvent::Type		s_confirmSleepEvent;
};

#endif
