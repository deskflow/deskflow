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

#ifndef CARCHNETWORKBSD_H
#define CARCHNETWORKBSD_H

#include "IArchNetwork.h"
#include "IArchMultithread.h"
#if HAVE_SYS_SOCKET_H
#	include <sys/socket.h>
#endif

#if !defined(HAVE_SOCKLEN_T)
typedef int socklen_t;
#endif

#define ARCH_NETWORK CArchNetworkBSD

class CArchSocketImpl {
public:
	int					m_fd;
	bool				m_connected;
	int					m_refCount;
};

class CArchNetAddressImpl {
public:
	CArchNetAddressImpl() : m_len(sizeof(m_addr)) { }

public:
	struct sockaddr		m_addr;
	socklen_t			m_len;
};

//! Berkeley (BSD) sockets implementation of IArchNetwork
class CArchNetworkBSD : public IArchNetwork {
public:
	CArchNetworkBSD();
	virtual ~CArchNetworkBSD();

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
	void				throwError(int);
	void				throwNameError(int);

private:
	CArchMutex			m_mutex;
};

#endif
