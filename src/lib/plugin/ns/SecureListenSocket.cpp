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

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "arch/XArch.h"

//
// SecureListenSocket
//

SecureListenSocket::SecureListenSocket(
		IEventQueue* events,
		SocketMultiplexer* socketMultiplexer) :
	TCPListenSocket(events, socketMultiplexer)
{
}

SecureListenSocket::~SecureListenSocket()
{
}

IDataSocket*
SecureListenSocket::accept()
{
	SecureSocket* socket = NULL;
	try {
		socket = new SecureSocket(
						m_events,
						m_socketMultiplexer,
						ARCH->acceptSocket(m_socket, NULL));
		socket->initSsl(true);
		// TODO: customized certificate path
		socket->loadCertificates("C:\\Temp\\synergy.pem");
		socket->secureAccept();

		if (socket != NULL) {
			m_socketMultiplexer->addSocket(this,
							new TSocketMultiplexerMethodJob<TCPListenSocket>(
								this, &TCPListenSocket::serviceListening,
								m_socket, true, false));
		}
		return dynamic_cast<IDataSocket*>(socket);
	}
	catch (XArchNetwork&) {
		if (socket != NULL) {
			delete socket;
		}
		return NULL;
	}
	catch (std::exception &ex) {
		if (socket != NULL) {
			delete socket;
		}
		throw ex;
	}
}
