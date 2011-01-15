/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef IDATASOCKET_H
#define IDATASOCKET_H

#include "ISocket.h"
#include "IStream.h"
#include "CString.h"

//! Data stream socket interface
/*!
This interface defines the methods common to all network sockets that
represent a full-duplex data stream.
*/
class IDataSocket : public ISocket, public IStream {
public:
	class CConnectionFailedInfo {
	public:
		CConnectionFailedInfo(const char* what) : m_what(what) { }
		CString			m_what;
	};

	//! @name manipulators
	//@{

	//! Connect socket
	/*!
	Attempt to connect to a remote endpoint.  This returns immediately
	and sends a connected event when successful or a connection failed
	event when it fails.  The stream acts as if shutdown for input and
	output until the stream connects.
	*/
	virtual void		connect(const CNetworkAddress&) = 0;

	//@}
	//! @name accessors
	//@{

	//! Get connected event type
	/*!
	Returns the socket connected event type.  A socket sends this
	event when a remote connection has been established.
	*/
	static CEvent::Type	getConnectedEvent();

	//! Get connection failed event type
	/*!
	Returns the socket connection failed event type.  A socket sends
	this event when an attempt to connect to a remote port has failed.
	The data is a pointer to a CConnectionFailedInfo.
	*/
	static CEvent::Type	getConnectionFailedEvent();

	//@}

	// ISocket overrides
	// close() and getEventTarget() aren't pure to work around a bug
	// in VC++6.  it claims the methods are unused locals and warns
	// that it's removing them.  it's presumably tickled by inheriting
	// methods with identical signatures from both superclasses.
	virtual void		bind(const CNetworkAddress&) = 0;
	virtual void		close();
	virtual void*		getEventTarget() const;

	// IStream overrides
	virtual UInt32		read(void* buffer, UInt32 n) = 0;
	virtual void		write(const void* buffer, UInt32 n) = 0;
	virtual void		flush() = 0;
	virtual void		shutdownInput() = 0;
	virtual void		shutdownOutput() = 0;
	virtual bool		isReady() const = 0;
	virtual UInt32		getSize() const = 0;

private:
	static CEvent::Type	s_connectedEvent;
	static CEvent::Type	s_failedEvent;
};

#endif
