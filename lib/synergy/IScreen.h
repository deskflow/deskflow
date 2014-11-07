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

#ifndef ISCREEN_H
#define ISCREEN_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "CEvent.h"

class IClipboard;

//! Screen interface
/*!
This interface defines the methods common to all screens.
*/
class IScreen : public IInterface {
public:
	struct CClipboardInfo {
	public:
		ClipboardID		m_id;
		UInt32			m_sequenceNumber;
	};

	//! @name accessors
	//@{

	//! Get event target
	/*!
	Returns the target used for events created by this object.
	*/
	virtual void*		getEventTarget() const = 0;

	//! Get clipboard
	/*!
	Save the contents of the clipboard indicated by \c id and return
	true iff successful.
	*/
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const = 0;

	//! Get screen shape
	/*!
	Return the position of the upper-left corner of the screen in \c x and
	\c y and the size of the screen in \c width and \c height.
	*/
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;

	//! Get cursor position
	/*!
	Return the current position of the cursor in \c x and \c y.
	*/
	virtual void		getCursorPos(SInt32& x, SInt32& y) const = 0;

	//! Get error event type
	/*!
	Returns the error event type.  This is sent whenever the screen has
	failed for some reason (e.g. the X Windows server died).
	*/
	static CEvent::Type	getErrorEvent();

	//! Get shape changed event type
	/*!
	Returns the shape changed event type.  This is sent whenever the
	screen's shape changes.
	*/
	static CEvent::Type	getShapeChangedEvent();

	//! Get clipboard grabbed event type
	/*!
	Returns the clipboard grabbed event type.  This is sent whenever the
	clipboard is grabbed by some other application so we don't own it
	anymore.  The data is a pointer to a CClipboardInfo.
	*/
	static CEvent::Type	getClipboardGrabbedEvent();

	//! Get suspend event type
	/*!
	Returns the suspend event type. This is sent whenever the system goes
	to sleep or a user session is deactivated (fast user switching).
	*/
	static CEvent::Type	getSuspendEvent();
	
	//! Get resume event type
	/*!
	Returns the suspend event type. This is sent whenever the system wakes
	up or a user session is activated (fast user switching).
	*/
	static CEvent::Type	getResumeEvent();
	
	//@}

private:
	static CEvent::Type	s_errorEvent;
	static CEvent::Type	s_shapeChangedEvent;
	static CEvent::Type	s_clipboardGrabbedEvent;
	static CEvent::Type	s_suspendEvent;
	static CEvent::Type	s_resumeEvent;
};

#endif
