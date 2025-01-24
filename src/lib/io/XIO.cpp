/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "io/XIO.h"

//
// XIOClosed
//

std::string XIOClosed::getWhat() const throw()
{
  return format("XIOClosed", "already closed");
}

//
// XIOEndOfStream
//

std::string XIOEndOfStream::getWhat() const throw()
{
  return format("XIOEndOfStream", "reached end of stream");
}

//
// XIOWouldBlock
//

std::string XIOWouldBlock::getWhat() const throw()
{
  return format("XIOWouldBlock", "stream operation would block");
}
