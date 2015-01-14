/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#pragma once

#include "net/TCPSocket.h"
#include "base/XBase.h"

class IEventQueue;
class SocketMultiplexer;

struct Ssl;

//! Generic socket exception
XBASE_SUBCLASS(XSecureSocket, XBase);


//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureSocket : public TCPSocket {
public:
	SecureSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer);
	SecureSocket(IEventQueue* events,
		SocketMultiplexer* socketMultiplexer,
		ArchSocket socket);
	~SecureSocket();

	// IStream overrides
	virtual UInt32		read(void* buffer, UInt32 n);
	virtual void		write(const void* buffer, UInt32 n);
	virtual bool		isReady() const;

	void				connectSecureSocket();
	void				acceptSecureSocket();
	void				initSsl(bool server);
	void				loadCertificates(const char* CertFile);

private:
	void				onConnected();

	// SSL
	void				initContext(bool server);
	void				createSSL();
	void				secureAccept(int s);
	void				secureConnect(int s);
	void				showCertificate();
	bool				checkResult(int n);
	void				showError();
	void				throwError(const char* reason);
	bool				getError();

private:
	Ssl*				m_ssl;
	bool				m_ready;
	char*				m_error;
};
