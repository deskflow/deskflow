/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "net/TCPSocket.h"
#include "net/XSocket.h"

class IEventQueue;
class SocketMultiplexer;
class ISocketMultiplexerJob;

struct Ssl;

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

	// ISocket overrides
	void				close();

	void				secureConnect();
	void				secureAccept();
	bool				isReady() const { return m_secureReady; }
	bool				isSecureReady();
	bool				isSecure() { return true; }
	UInt32				secureRead(void* buffer, UInt32 n);
	UInt32				secureWrite(const void* buffer, UInt32 n);
	void				initSsl(bool server);
	bool				loadCertificates(String& CertFile);

private:
	// SSL
	void				initContext(bool server);
	void				createSSL();
	bool				secureAccept(int s);
	bool				secureConnect(int s);
	bool				showCertificate();
	void				checkResult(int n, bool& fatal, bool& retry);
	void				showError(const char* reason = NULL);
	String				getError();
	void				disconnect();
	void				formatFingerprint(String& fingerprint,
											bool hex = true,
											bool separator = true);
	bool				verifyCertFingerprint();

	ISocketMultiplexerJob*
						serviceConnect(ISocketMultiplexerJob*,
							bool, bool, bool);

	ISocketMultiplexerJob*
						serviceAccept(ISocketMultiplexerJob*,
							bool, bool, bool);

private:
	Ssl*				m_ssl;
	bool				m_secureReady;
};
