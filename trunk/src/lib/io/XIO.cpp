/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "XIO.h"

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


//
// XIOWouldBlock
//

CString
XIOWouldBlock::getWhat() const throw()
{
	return format("XIOWouldBlock", "stream operation would block");
}
