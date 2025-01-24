/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/IDataSocket.h"
#include "base/EventQueue.h"

//
// IDataSocket
//

void IDataSocket::close()
{
  // this is here to work around a VC++6 bug.  see the header file.
  assert(0 && "bad call");
}

void *IDataSocket::getEventTarget() const
{
  // this is here to work around a VC++6 bug.  see the header file.
  assert(0 && "bad call");
  return NULL;
}
