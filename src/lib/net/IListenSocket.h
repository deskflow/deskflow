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

#include "net/ISocket.h"
#include "base/EventTypes.h"

class IDataSocket;

//! Listen socket interface
/*!
This interface defines the methods common to all network sockets that
listen for incoming connections.
*/
class IListenSocket : public ISocket {
public:
    //! @name manipulators
    //@{

    //! Accept connection
    /*!
    Accept a connection, returning a socket representing the full-duplex
    data stream.  Returns NULL if no socket is waiting to be accepted.
    This is only valid after a call to \c bind().
    */
    virtual IDataSocket* accept () = 0;

    //@}

    // ISocket overrides
    virtual void bind (const NetworkAddress&) = 0;
    virtual void close ()                     = 0;
    virtual void* getEventTarget () const     = 0;
};
