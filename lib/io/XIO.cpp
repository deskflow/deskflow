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

#include "XIO.h"

//
// XIOErrno
//

XIOErrno::XIOErrno() :
	MXErrno()
{
	// do nothing
}

XIOErrno::XIOErrno(int err) :
	MXErrno(err)
{
	// do nothing
}


//
// XIOClose
//

CString
XIOClose::getWhat() const throw()
{
	return format("XIOClose", "close: %{1}", XIOErrno::getErrstr());
}


//
// XIOClosed
//

CString
XIOClosed::getWhat() const throw()
{
	return format("XIOClosed", "already closed");
}


//
// XIOEndOfStream
//

CString
XIOEndOfStream::getWhat() const throw()
{
	return format("XIOEndOfStream", "reached end of stream");
}
