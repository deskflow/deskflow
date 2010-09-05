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

#ifndef ICLIPBOARD_H
#define ICLIPBOARD_H

#include "IInterface.h"
#include "CString.h"
#include "BasicTypes.h"

//! Clipboard interface
/*!
This interface defines the methods common to all clipboards.
*/
class IClipboard : public IInterface {
public:
	//! Timestamp type
	/*!
	Timestamp type.  Timestamps are in milliseconds from some
	arbitrary starting time.  Timestamps will wrap around to 0
	after about 49 3/4 days.
	*/
	typedef UInt32 Time;

	//! Clipboard formats
	/*!
	The list of known clipboard formats.  kNumFormats must be last and
	formats must be sequential starting from zero.  Clipboard data set
	via add() and retrieved via get() must be in one of these formats.
	Platform dependent clipboard subclasses can and should present any
	suitable formats derivable from these formats.
	*/
	enum EFormat {
		kText,			//!< Text format, UTF-8, newline is LF
		kNumFormats		//!< The number of clipboard formats
	};

	//! @name manipulators
	//@{

	//! Empty clipboard
	/*!
	Take ownership of the clipboard and clear all data from it.
	This must be called between a successful open() and close().
	Return false if the clipboard ownership could not be taken;
	the clipboard should not be emptied in this case.
	*/
	virtual bool		empty() = 0;

	//! Add data
	/*!
	Add data in the given format to the clipboard.  May only be
	called after a successful empty().
	*/
	virtual void		add(EFormat, const CString& data) = 0;

	//@}
	//! @name accessors
	//@{

	//! Open clipboard
	/*!
	Open the clipboard.  Return true iff the clipboard could be
	opened.  If open() returns true then the client must call
	close() at some later time;  if it returns false then close()
	must not be called.  \c time should be the current time or
	a time in the past when the open should effectively have taken
	place.
	*/
	virtual bool		open(Time time) const = 0;

	//! Close clipboard
	/*!
	Close the clipboard.  close() must match a preceding successful
	open().  This signals that the clipboard has been filled with
	all the necessary data or all data has been read.  It does not
	mean the clipboard ownership should be released (if it was
	taken).
	*/
	virtual void		close() const = 0;

	//! Get time
	/*!
	Return the timestamp passed to the last successful open().
	*/
	virtual Time		getTime() const = 0;

	//! Check for data
	/*!
	Return true iff the clipboard contains data in the given
	format.  Must be called between a successful open() and close().
	*/
	virtual bool		has(EFormat) const = 0;

	//! Get data
	/*!
	Return the data in the given format.  Returns the empty string
	if there is no data in that format.  Must be called between
	a successful open() and close().
	*/
	virtual CString		get(EFormat) const = 0;

	//@}
};

#endif
