/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CARCHNETWORKWINSOCK_H
#define CARCHNETWORKWINSOCK_H

#define WIN32_LEAN_AND_MEAN

// declare no functions in winsock2
#define INCL_WINSOCK_API_PROTOTYPES 0
#define INCL_WINSOCK_API_TYPEDEFS 0

#include "IArchNetwork.h"
#include "IArchMultithread.h"
#include <windows.h>
#include <winsock2.h>

#define ARCH_NETWORK CArchNetworkWinsock

class CArchSocketImpl {
public:
	SOCKET				m_socket;
	bool				m_connected;
	int					m_refCount;
};

class CArchNetAddressImpl {
public:
	CArchNetAddressImpl() : m_len(sizeof(m_addr)) { }

public:
	struct sockaddr		m_addr;
	int					m_len;
};

//! Win32 implementation of IArchNetwork
class CArchNetworkWinsock : public IArchNetwork {
public:
	CArchNetworkWinsock();
	virtual ~CArchNetworkWinsock();

	// IArchNetwork overrides
	virtual CArchSocket	newSocket(EAddressFamily, ESocketType);
	virtual CArchSocket	copySocket(CArchSocket s);
	virtual void		closeSocket(CArchSocket s);
	virtual void		closeSocketForRead(CArchSocket s);
	virtual void		closeSocketForWrite(CArchSocket s);
	virtual void		bindSocket(CArchSocket s, CArchNetAddress addr);
	virtual void		listenOnSocket(CArchSocket s);
	virtual CArchSocket	acceptSocket(CArchSocket s, CArchNetAddress* addr);
	virtual void		connectSocket(CArchSocket s, CArchNetAddress name);
	virtual int			pollSocket(CPollEntry[], int num, double timeout);
	virtual size_t		readSocket(CArchSocket s, void* buf, size_t len);
	virtual size_t		writeSocket(CArchSocket s,
							const void* buf, size_t len);
	virtual void		throwErrorOnSocket(CArchSocket);
	virtual bool		setBlockingOnSocket(CArchSocket, bool blocking);
	virtual bool		setNoDelayOnSocket(CArchSocket, bool noDelay);
	virtual std::string		getHostName();
	virtual CArchNetAddress	newAnyAddr(EAddressFamily);
	virtual CArchNetAddress	copyAddr(CArchNetAddress);
	virtual CArchNetAddress	nameToAddr(const std::string&);
	virtual void			closeAddr(CArchNetAddress);
	virtual std::string		addrToName(CArchNetAddress);
	virtual std::string		addrToString(CArchNetAddress);
	virtual EAddressFamily	getAddrFamily(CArchNetAddress);
	virtual void			setAddrPort(CArchNetAddress, int port);
	virtual int				getAddrPort(CArchNetAddress);
	virtual bool			isAnyAddr(CArchNetAddress);

private:
	void				init(HMODULE);

	void				throwError(int);
	void				throwNameError(int);

private:
	CArchMutex			m_mutex;
};

#endif
