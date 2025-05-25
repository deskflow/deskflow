/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2019 Barrier Contributors
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "common/IInterface.h"

#include <memory>

class ISocketMultiplexerJob;
struct MultiplexerJobStatus
{
  MultiplexerJobStatus(bool cont, std::unique_ptr<ISocketMultiplexerJob> &&nj)
      : continueServicing(cont),
        newJob(std::move(nj))
  {
    // do nothing
  }
  bool continueServicing = false;
  std::unique_ptr<ISocketMultiplexerJob> newJob;
};

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
  Called by a socket multiplexer when the socket becomes readable, writable, or has an error.
  The socket is readable if \p readable is true, writable if \p writable is true, and in error
  if \p error is true.
  The method returns false as the continue_servicing member of the returned struct if the socket
  should no longer be served and true otherwise. Additionally, if the newJob member of the
  returned pair is not empty, the socket should be serviced differently with the specified job.

  This call must not attempt to directly change the job for this
  socket by calling \c addSocket() or \c removeSocket() on the
  multiplexer.  It must instead return the new job.  It can,
  however, add or remove jobs for other sockets.
  */
  virtual MultiplexerJobStatus run(bool readable, bool writable, bool error) = 0;

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

  virtual bool isCursor() const
  {
    return false;
  }

  //@}
};
