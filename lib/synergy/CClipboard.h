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

#ifndef CCLIPBOARD_H
#define CCLIPBOARD_H

#include "IClipboard.h"

//! Memory buffer clipboard
/*!
This class implements a clipboard that stores data in memory.
*/
class CClipboard : public IClipboard {
public:
	CClipboard();
	virtual ~CClipboard();

	//! @name manipulators
	//@{

	//! Unmarshall clipboard data
	/*!
	Extract marshalled clipboard data and store it in this clipboard.
	Sets the clipboard time to \c time.
	*/
	void				unmarshall(const CString& data, Time time);

	//@}
	//! @name accessors
	//@{

	//! Marshall clipboard data
	/*!
	Merge this clipboard's data into a single buffer that can be later
	unmarshalled to restore the clipboard and return the buffer.
	*/
	CString				marshall() const;

	//! Copy clipboard
	/*!
	Transfers all the data in one clipboard to another.  The
	clipboards can be of any concrete clipboard type (and
	they don't have to be the same type).  This also sets
	the destination clipboard's timestamp to source clipboard's
	timestamp.  Returns true iff the copy succeeded.
	*/
	static bool			copy(IClipboard* dst, const IClipboard* src);

	//! Copy clipboard
	/*!
	Transfers all the data in one clipboard to another.  The
	clipboards can be of any concrete clipboard type (and they
	don't have to be the same type).  This also sets the
	timestamp to \c time.  Returns true iff the copy succeeded.
	*/
	static bool			copy(IClipboard* dst, const IClipboard* src, Time);

	//@}

	// IClipboard overrides
	virtual bool		empty();
	virtual void		add(EFormat, const CString& data);
	virtual bool		open(Time) const;
	virtual void		close() const;
	virtual Time		getTime() const;
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;

private:
	UInt32				readUInt32(const char*) const;
	void				writeUInt32(CString*, UInt32) const;

private:
	mutable bool		m_open;
	mutable Time		m_time;
	bool				m_owner;
	Time				m_timeOwned;
	bool				m_added[kNumFormats];
	CString				m_data[kNumFormats];
};

#endif
