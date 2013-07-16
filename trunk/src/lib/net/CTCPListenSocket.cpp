/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CTCPListenSocket.h"
#include "CNetworkAddress.h"
#include "CSocketMultiplexer.h"
#include "CTCPSocket.h"
#include "TSocketMultiplexerMethodJob.h"
#include "XSocket.h"
#include "XIO.h"
#include "CLock.h"
#include "CMutex.h"
#include "IEventQueue.h"
#include "CArch.h"
#include "XArch.h"

//
// CTCPListenSocket
//

CTCPListenSocket::CTCPListenSocket(IEventQueue* events, CSocketMultiplexer* socketMultiplexer) :
	m_events(events),
	m_socketMultiplexer(socketMultiplexer)
{
	m_mutex = new CMutex;
	try {
		m_socket = ARCH->newSocket(IArchNetwork::kINET, IArchNetwork::kSTREAM);
	}
	catch (XArchNetwork& e) {
		throw XSocketCreate(e.what());
	}
}

CTCPListenSocket::~CTCPListenSocket()
{
	try {
		if (m_socket != NULL) {
			m_socketMultiplexer->removeSocket(this);
			ARCH->closeSocket(m_socket);
		}
	}
	catch (...) {
		// ignore
	}
	delete m_mutex;
}

void
CTCPListenSocket::bind(const CNetworkAddress& addr)
{
	try {
		CLock lock(m_mutex);
		ARCH->setReuseAddrOnSocket(m_socket, true);
		ARCH->bindSocket(m_socket, addr.getAddress());
		ARCH->listenOnSocket(m_socket);
		m_socketMultiplexer->addSocket(this,
							new TSocketMultiplexerMethodJob<CTCPListenSocket>(
								this, &CTCPListenSocket::serviceListening,
								m_socket, true, false));
	}
	catch (XArchNetworkAddressInUse& e) {
		throw XSocketAddressInUse(e.what());
	}
	catch (XArchNetwork& e) {
		throw XSocketBind(e.what());
	}
}

void
CTCPListenSocket::close()
{
	CLock lock(m_mutex);
	if (m_socket == NULL) {
		throw XIOClosed();
	}
	try {
		m_socketMultiplexer->removeSocket(this);
		ARCH->closeSocket(m_socket);
		m_socket = NULL;
	}
	catch (XArchNetwork& e) {
		throw XSocketIOClose(e.what());
	}
}

void*
CTCPListenSocket::getEventTarget() const
{
	return const_cast<void*>(reinterpret_cast<const void*>(this));
}

IDataSocket*
CTCPListenSocket::accept()
{
	IDataSocket* socket = NULL;
	try {
		socket = new CTCPSocket(m_events, m_socketMultiplexer, ARCH->acceptSocket(m_socket, NULL));
		if (socket != NULL) {
			m_socketMultiplexer->addSocket(this,
							new TSocketMultiplexerMethodJob<CTCPListenSocket>(
								this, &CTCPListenSocket::serviceListening,
								m_socket, true, false));
		}
		return socket;
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

ISocketMultiplexerJob*
CTCPListenSocket::serviceListening(ISocketMultiplexerJob* job,
							bool read, bool, bool error)
{
	if (error) {
		close();
		return NULL;
	}
	if (read) {
		m_events->addEvent(CEvent(m_events->forIListenSocket().connecting(), this, NULL));
		// stop polling on this socket until the client accepts
		return NULL;
	}
	return job;
}
