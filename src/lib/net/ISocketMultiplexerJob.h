/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "common/IInterface.h"

//! Socket multiplexer job
/*!
A socket multiplexer job handles events on a socket.
*/
class ISocketMultiplexerJob : public IInterface
{
public:
  //! @name manipulators
  //@{

  //! Handle socket event
  /*!
  Called by a socket multiplexer when the socket becomes readable,
  writable, or has an error.  It should return itself if the same
  job can continue to service events, a new job if the socket must
  be serviced differently, or NULL if the socket should no longer
  be serviced.  The socket is readable if \p readable is true,
  writable if \p writable is true, and in error if \p error is
  true.

  This call must not attempt to directly change the job for this
  socket by calling \c addSocket() or \c removeSocket() on the
  multiplexer.  It must instead return the new job.  It can,
  however, add or remove jobs for other sockets.
  */
  virtual ISocketMultiplexerJob *run(bool readable, bool writable, bool error) = 0;

  //@}
  //! @name accessors
  //@{

  //! Get the socket
  /*!
  Return the socket to multiplex
  */
  virtual ArchSocket getSocket() const = 0;

  //! Check for interest in readability
  /*!
  Return true if the job is interested in being run if the socket
  becomes readable.
  */
  virtual bool isReadable() const = 0;

  //! Check for interest in writability
  /*!
  Return true if the job is interested in being run if the socket
  becomes writable.
  */
  virtual bool isWritable() const = 0;

  //@}
};
