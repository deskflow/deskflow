/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef IARCHNETWORK_H
#define IARCHNETWORK_H

#include "IInterface.h"
#include "stdstring.h"

class CArchSocketImpl;
class CArchNetAddressImpl;
typedef CArchSocketImpl* CArchSocket;
typedef CArchNetAddressImpl* CArchNetAddress;

//! Interface for architecture dependent networking
/*!
This interface defines the networking operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchNetwork : public IInterface {
public:
	enum EAddressFamily {
		kUNKNOWN,
		kINET,
	};

	enum ESocketType {
		kDGRAM,
		kSTREAM
	};

	enum {
		kPOLLIN   = 1,
		kPOLLOUT  = 2,
		kPOLLERR  = 4,
		kPOLLNVAL = 8
	};

	class CPollEntry {
	public:
		CArchSocket		m_socket;
		unsigned short	m_events;
		unsigned short	m_revents;
	};

	//! @name manipulators
	//@{

	virtual CArchSocket	newSocket(EAddressFamily, ESocketType) = 0;
	virtual CArchSocket	copySocket(CArchSocket s) = 0;
	virtual void		closeSocket(CArchSocket s) = 0;
	virtual void		closeSocketForRead(CArchSocket s) = 0;
	virtual void		closeSocketForWrite(CArchSocket s) = 0;
	virtual void		bindSocket(CArchSocket s, CArchNetAddress addr) = 0;
	virtual void		listenOnSocket(CArchSocket s) = 0;
	virtual CArchSocket	acceptSocket(CArchSocket s, CArchNetAddress* addr) = 0;
	virtual void		connectSocket(CArchSocket s, CArchNetAddress name) = 0;
	virtual int			pollSocket(CPollEntry[], int num, double timeout) = 0;
	virtual size_t		readSocket(CArchSocket s, void* buf, size_t len) = 0;
	virtual size_t		writeSocket(CArchSocket s,
							const void* buf, size_t len) = 0;
	virtual void		throwErrorOnSocket(CArchSocket) = 0;

	//! Set socket to (non-)blocking operation
	/*!
	Set socket to block or not block on accept, connect, poll, read and
	write (i.e. calls that may take an arbitrary amount of time).
	Returns the previous state.
	*/
	virtual bool		setBlockingOnSocket(CArchSocket, bool blocking) = 0;

	//! Turn Nagle algorithm on or off on socket
	/*!
	Set socket to send messages immediately (true) or to collect small
	messages into one packet (false).  Returns the previous state.
	*/
	virtual bool		setNoDelayOnSocket(CArchSocket, bool noDelay) = 0;

	virtual std::string		getHostName() = 0;
	virtual CArchNetAddress	newAnyAddr(EAddressFamily) = 0;
	virtual CArchNetAddress	copyAddr(CArchNetAddress) = 0;
	virtual CArchNetAddress	nameToAddr(const std::string&) = 0;
	virtual void			closeAddr(CArchNetAddress) = 0;
	virtual std::string		addrToName(CArchNetAddress) = 0;
	virtual std::string		addrToString(CArchNetAddress) = 0;
	virtual EAddressFamily	getAddrFamily(CArchNetAddress) = 0;
	virtual void			setAddrPort(CArchNetAddress, int port) = 0;
	virtual int				getAddrPort(CArchNetAddress) = 0;
	virtual bool			isAnyAddr(CArchNetAddress) = 0;

	//@}
};

#endif
