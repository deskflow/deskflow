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

#ifndef IPLATFORMSCREEN_H
#define IPLATFORMSCREEN_H

#include "IScreen.h"
#include "IPrimaryScreen.h"
#include "ISecondaryScreen.h"
#include "ClipboardTypes.h"
#include "OptionTypes.h"
#include "CEvent.h"

class IClipboard;
class IKeyState;

//! Screen interface
/*!
This interface defines the methods common to all platform dependent
screen implementations that are used by both primary and secondary
screens.  A platform screen is expected to post the events defined
in \c IScreen when appropriate.  It should also post events defined
in \c IPlatformScreen if acting as the primary screen.  The target
on the events should be the value returned by \c getEventTarget().
*/
class IPlatformScreen : public IScreen,
				public IPrimaryScreen, public ISecondaryScreen {
public:
	//! Key event data
	class CKeyInfo {
	public:
		static CKeyInfo* alloc(KeyID, KeyModifierMask, KeyButton, SInt32 count);

	public:
		KeyID			m_key;
		KeyModifierMask	m_mask;
		KeyButton		m_button;
		SInt32			m_count;
	};
	//! Button event data
	class CButtonInfo {
	public:
		static CButtonInfo* alloc(ButtonID);

	public:
		ButtonID		m_button;
	};
	//! Motion event data
	class CMotionInfo {
	public:
		static CMotionInfo* alloc(SInt32 x, SInt32 y);

	public:
		SInt32			m_x;
		SInt32			m_y;
	};
	//! Wheel motion event data
	class CWheelInfo {
	public:
		static CWheelInfo* alloc(SInt32);

	public:
		SInt32			m_wheel;
	};

	//! @name manipulators
	//@{

	//! Set the key state
	/*!
	Sets the key state object.  This object tracks keyboard state and
	the screen is expected to keep it up to date.
	*/
	virtual void		setKeyState(IKeyState*) = 0;

	//! Enable screen
	/*!
	Enable the screen, preparing it to report system and user events.
	For a secondary screen it also means preparing to synthesize events
	and hiding the cursor.
	*/
	virtual void		enable() = 0;

	//! Disable screen
	/*!
	Undoes the operations in enable() and events should no longer
	be reported.
	*/
	virtual void		disable() = 0;

	//! Enter screen
	/*!
	Called when the user navigates to this screen.
	*/
	virtual void		enter() = 0;

	//! Leave screen
	/*!
	Called when the user navigates off the screen.  Returns true on
	success, false on failure.  A typical reason for failure is being
	unable to install the keyboard and mouse snoopers on a primary
	screen.  Secondary screens should not fail.
	*/
	virtual bool		leave() = 0;

	//! Set clipboard
	/*!
	Set the contents of the system clipboard indicated by \c id.
	*/
	virtual bool		setClipboard(ClipboardID id, const IClipboard*) = 0;

	//! Check clipboard owner
	/*!
	Check ownership of all clipboards and post grab events for any that
	have changed.  This is used as a backup in case the system doesn't
	reliably report clipboard ownership changes.
	*/
	virtual void		checkClipboards() = 0;

	//! Open screen saver
	/*!
	Open the screen saver.  If \c notify is true then this object must
	send events when the screen saver activates or deactivates until
	\c closeScreensaver() is called.  If \c notify is false then the
	screen saver is disabled and restored on \c closeScreensaver().
	*/
	virtual void		openScreensaver(bool notify) = 0;

	//! Close screen saver
	/*!
	// Close the screen saver.  Stop reporting screen saver activation
	and deactivation and, if the screen saver was disabled by
	openScreensaver(), enable the screen saver.
	*/
	virtual void		closeScreensaver() = 0;

	//! Activate/deactivate screen saver
	/*!
	Forcibly activate the screen saver if \c activate is true otherwise
	forcibly deactivate it.
	*/
	virtual void		screensaver(bool activate) = 0;

	//! Notify of options changes
	/*!
	Reset all options to their default values.
	*/
	virtual void		resetOptions() = 0;

	//! Notify of options changes
	/*!
	Set options to given values.  Ignore unknown options and don't
	modify options that aren't given in \c options.
	*/
	virtual void		setOptions(const COptionsList& options) = 0;

	//! Get keyboard state
	/*!
	Put the current keyboard state into the IKeyState passed to \c open().
	*/
	virtual void		updateKeys() = 0;

	//! Set clipboard sequence number
	/*!
	Sets the sequence number to use in subsequent clipboard events.
	*/
	virtual void		setSequenceNumber(UInt32) = 0;

	//@}
	//! @name accessors
	//@{

	//! Test if is primary screen
	/*!
	Return true iff this screen is a primary screen.
	*/
	virtual bool		isPrimary() const = 0;

	//! Get key down event type.  Event data is CKeyInfo*, count == 1.
	static CEvent::Type	getKeyDownEvent();
	//! Get key up event type.  Event data is CKeyInfo*, count == 1.
	static CEvent::Type	getKeyUpEvent();
	//! Get key repeat event type.  Event data is CKeyInfo*.
	static CEvent::Type	getKeyRepeatEvent();
	//! Get button down event type.  Event data is CButtonInfo*.
	static CEvent::Type	getButtonDownEvent();
	//! Get button up event type.  Event data is CButtonInfo*.
	static CEvent::Type	getButtonUpEvent();
	//! Get mouse motion on the primary screen event type
	/*!
	Event data is CMotionInfo* and the values are an absolute position.
	*/
	static CEvent::Type	getMotionOnPrimaryEvent();
	//! Get mouse motion on a secondary screen event type
	/*!
	Event data is CMotionInfo* and the values are motion deltas not
	absolute coordinates.
	*/
	static CEvent::Type	getMotionOnSecondaryEvent();
	//! Get mouse wheel event type.  Event data is CWheelInfo*.
	static CEvent::Type	getWheelEvent();
	//! Get screensaver activated event type
	static CEvent::Type	getScreensaverActivatedEvent();
	//! Get screensaver deactivated event type
	static CEvent::Type	getScreensaverDeactivatedEvent();

	//@}

	// IScreen overrides
	virtual void*		getEventTarget() const = 0;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const = 0;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const = 0;

	// IPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides) = 0;
	virtual void		warpCursor(SInt32 x, SInt32 y) = 0;
	virtual SInt32		getJumpZoneSize() const = 0;
	virtual bool		isAnyMouseButtonDown() const = 0;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const = 0;
	virtual const char*	getKeyName(KeyButton) const = 0;

	// ISecondaryScreen overrides
	virtual void		fakeKeyEvent(KeyButton id, bool press) const = 0;
	virtual bool		fakeCtrlAltDel() const = 0;
	virtual void		fakeMouseButton(ButtonID id, bool press) const = 0;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const = 0;
	virtual void		fakeMouseWheel(SInt32 delta) const = 0;
	virtual KeyButton	mapKey(IKeyState::Keystrokes&,
							const IKeyState& keyState, KeyID id,
							KeyModifierMask desiredMask,
							bool isAutoRepeat) const = 0;

private:
	static CEvent::Type	s_keyDownEvent;
	static CEvent::Type	s_keyUpEvent;
	static CEvent::Type	s_keyRepeatEvent;
	static CEvent::Type	s_buttonDownEvent;
	static CEvent::Type	s_buttonUpEvent;
	static CEvent::Type	s_motionPrimaryEvent;
	static CEvent::Type	s_motionSecondaryEvent;
	static CEvent::Type	s_wheelEvent;
	static CEvent::Type	s_ssActivatedEvent;
	static CEvent::Type	s_ssDeactivatedEvent;
};

#endif
