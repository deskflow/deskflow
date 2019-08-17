/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "arch/IArchNetwork.h"
#include "common/IInterface.h"
#include <memory>

class ISocketMultiplexerJob;

struct MultiplexerJobStatus
{
    MultiplexerJobStatus(bool cont, std::unique_ptr<ISocketMultiplexerJob>&& nj) :
        continue_servicing(cont), new_job(std::move(nj))
    {}

    bool continue_servicing = false;
    std::unique_ptr<ISocketMultiplexerJob> new_job;
};

//! Socket multiplexer job
/*!
A socket multiplexer job handles events on a socket.
*/
class ISocketMultiplexerJob : public IInterface {
public:
    //! @name manipulators
    //@{

    //! Handle socket event
    /*!
    Called by a socket multiplexer when the socket becomes readable, writable, or has an error.
    The socket is readable if \p readable is true, writable if \p writable is true, and in error
    if \p error is true.

    The method returns false as the continue_servicing member of the returned struct if the socket
    should no longer be served and true otherwise. Additionally, if the new_job member of the
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
    virtual ArchSocket    getSocket() const = 0;

    //! Check for interest in readability
    /*!
    Return true if the job is interested in being run if the socket
    becomes readable.
    */
    virtual bool        isReadable() const = 0;

    //! Check for interest in writability
    /*!
    Return true if the job is interested in being run if the socket
    becomes writable.
    */
    virtual bool        isWritable() const = 0;

    virtual bool isCursor() const { return false; }
    //@}
};
