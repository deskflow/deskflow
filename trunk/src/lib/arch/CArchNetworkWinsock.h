/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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
	int					m_refCount;
	WSAEVENT			m_event;
	bool				m_pollWrite;
};

class CArchNetAddressImpl {
public:
	static CArchNetAddressImpl* alloc(size_t);

public:
	int					m_len;
	struct sockaddr		m_addr;
};
#define ADDR_HDR_SIZE	offsetof(CArchNetAddressImpl, m_addr)
#define TYPED_ADDR(type_, addr_) (reinterpret_cast<type_*>(&addr_->m_addr))

//! Win32 implementation of IArchNetwork
class CArchNetworkWinsock : public IArchNetwork {
public:
	CArchNetworkWinsock();
	virtual ~CArchNetworkWinsock();

	virtual void init();

	// IArchNetwork overrides
	virtual CArchSocket	newSocket(EAddressFamily, ESocketType);
	virtual CArchSocket	copySocket(CArchSocket s);
	virtual void		closeSocket(CArchSocket s);
	virtual void		closeSocketForRead(CArchSocket s);
	virtual void		closeSocketForWrite(CArchSocket s);
	virtual void		bindSocket(CArchSocket s, CArchNetAddress addr);
	virtual void		listenOnSocket(CArchSocket s);
	virtual CArchSocket	acceptSocket(CArchSocket s, CArchNetAddress* addr);
	virtual bool		connectSocket(CArchSocket s, CArchNetAddress name);
	virtual int			pollSocket(CPollEntry[], int num, double timeout);
	virtual void		unblockPollSocket(CArchThread thread);
	virtual size_t		readSocket(CArchSocket s, void* buf, size_t len);
	virtual size_t		writeSocket(CArchSocket s,
							const void* buf, size_t len);
	virtual void		throwErrorOnSocket(CArchSocket);
	virtual bool		setNoDelayOnSocket(CArchSocket, bool noDelay);
	virtual bool		setReuseAddrOnSocket(CArchSocket, bool reuse);
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
	virtual bool			isEqualAddr(CArchNetAddress, CArchNetAddress);

private:
	void				initModule(HMODULE);

	void				setBlockingOnSocket(SOCKET, bool blocking);

	void				throwError(int);
	void				throwNameError(int);

private:
	CArchMutex			m_mutex;
};

#endif
