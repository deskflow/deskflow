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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
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
	bool				isFatal() const { return m_fatal; }
	void				isFatal(bool b) { m_fatal = b; }
	bool				isSecureReady();
	bool				isSecure() { return true; }
	int					secureRead(void* buffer, int size, int& read);
	int					secureWrite(const void* buffer, int size, int& wrote);
	void				initSsl(bool server);
	bool				loadCertificates(String& CertFile);

private:
	// SSL
	void				initContext(bool server);
	void				createSSL();
	int					secureAccept(int s);
	int					secureConnect(int s);
	bool				showCertificate();
	void				checkResult(int n, int& retry);
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

	void				showSecureConnectInfo();
	void				showSecureLibInfo();
	void				showSecureCipherInfo();

private:
	Ssl*				m_ssl;
	bool				m_secureReady;
	bool				m_fatal;
};
