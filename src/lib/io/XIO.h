/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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
 */

#pragma once

#include "base/XBase.h"

//! Generic I/O exception
XBASE_SUBCLASS (XIO, XBase);

//! I/O closing exception
/*!
Thrown if a stream cannot be closed.
*/
XBASE_SUBCLASS (XIOClose, XIO);

//! I/O already closed exception
/*!
Thrown when attempting to close or perform I/O on an already closed.
stream.
*/
XBASE_SUBCLASS_WHAT (XIOClosed, XIO);

//! I/O end of stream exception
/*!
Thrown when attempting to read beyond the end of a stream.
*/
XBASE_SUBCLASS_WHAT (XIOEndOfStream, XIO);

//! I/O would block exception
/*!
Thrown if an operation on a stream would block.
*/
XBASE_SUBCLASS_WHAT (XIOWouldBlock, XIO);
