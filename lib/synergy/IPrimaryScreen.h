/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#ifndef IPRIMARYSCREEN_H
#define IPRIMARYSCREEN_H

#include "IInterface.h"
#include "MouseTypes.h"
#include "CEvent.h"

//! Primary screen interface
/*!
This interface defines the methods common to all platform dependent
primary screen implementations.
*/
class IPrimaryScreen : public IInterface {
public:
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

	//! Update configuration
	/*!
	This is called when the configuration has changed.  \c activeSides
	is a bitmask of EDirectionMask indicating which sides of the
	primary screen are linked to clients.  Override to handle the
	possible change in jump zones.
	*/
	virtual void		reconfigure(UInt32 activeSides) = 0;

	//! Warp cursor
	/*!
	Warp the cursor to the absolute coordinates \c x,y.  Also
	discard input events up to and including the warp before
	returning.
	*/
	virtual void		warpCursor(SInt32 x, SInt32 y) = 0;

	//@}
	//! @name accessors
	//@{

	//! Get jump zone size
	/*!
	Return the jump zone size, the size of the regions on the edges of
	the screen that cause the cursor to jump to another screen.
	*/
	virtual SInt32		getJumpZoneSize() const = 0;

	//! Test if mouse is pressed
	/*!
	Return true if any mouse button is currently pressed.  Ideally,
	"current" means up to the last processed event but it can mean
	the current physical mouse button state.
	*/
	virtual bool		isAnyMouseButtonDown() const = 0;

	//! Get cursor center position
	/*!
	Return the cursor center position which is where we park the
	cursor to compute cursor motion deltas and should be far from
	the edges of the screen, typically the center.
	*/
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const = 0;

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

private:
	static CEvent::Type	s_buttonDownEvent;
	static CEvent::Type	s_buttonUpEvent;
	static CEvent::Type	s_motionPrimaryEvent;
	static CEvent::Type	s_motionSecondaryEvent;
	static CEvent::Type	s_wheelEvent;
	static CEvent::Type	s_ssActivatedEvent;
	static CEvent::Type	s_ssDeactivatedEvent;
};

#endif
