/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "io/IOException.h"

//
// IOClosedException
//

std::string IOClosedException::getWhat() const throw()
{
  return format("IOClosedException", "already closed");
}

//
// IOEndOfStreamException
//

std::string IOEndOfStreamException::getWhat() const throw()
{
  return format("IOEndOfStreamException", "reached end of stream");
}

//
// IOWouldBlockException
//

std::string IOWouldBlockException::getWhat() const throw()
{
  return format("IOWouldBlockException", "stream operation would block");
}
