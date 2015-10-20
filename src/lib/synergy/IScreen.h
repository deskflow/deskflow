/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2003 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "synergy/clipboard_types.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include "common/IInterface.h"

class IClipboard;

//! Screen interface
/*!
This interface defines the methods common to all screens.
*/
class IScreen : public IInterface {
public:
	struct ClipboardInfo {
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
