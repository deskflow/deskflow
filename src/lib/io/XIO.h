/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

//! Generic I/O exception
XBASE_SUBCLASS(XIO, XBase);

//! I/O closing exception
/*!
Thrown if a stream cannot be closed.
*/
XBASE_SUBCLASS(XIOClose, XIO);

//! I/O already closed exception
/*!
Thrown when attempting to close or perform I/O on an already closed.
stream.
*/
XBASE_SUBCLASS_WHAT(XIOClosed, XIO);

//! I/O end of stream exception
/*!
Thrown when attempting to read beyond the end of a stream.
*/
XBASE_SUBCLASS_WHAT(XIOEndOfStream, XIO);

//! I/O would block exception
/*!
Thrown if an operation on a stream would block.
*/
XBASE_SUBCLASS_WHAT(XIOWouldBlock, XIO);
