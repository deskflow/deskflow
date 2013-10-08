/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ISCREEN_H
#define ISCREEN_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "CEvent.h"
#include "CEventTypes.h"

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
	
	//@}
};

#endif
