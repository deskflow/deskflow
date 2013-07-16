/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "CIpcServer.h"
#include "Ipc.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "CEvent.h"
#include "CLog.h"
#include "CIpcClientProxy.h"
#include "IStream.h"
#include "IDataSocket.h"
#include "CIpcMessage.h"

//
// CIpcServer
//

CIpcServer::CIpcServer(IEventQueue* events, CSocketMultiplexer* socketMultiplexer) :
	m_events(events),
	m_socket(events, socketMultiplexer),
	m_address(CNetworkAddress(IPC_HOST, IPC_PORT))
{
	init();
}

CIpcServer::CIpcServer(IEventQueue* events, CSocketMultiplexer* socketMultiplexer, int port) :
	m_events(events),
	m_socket(events, socketMultiplexer),
	m_address(CNetworkAddress(IPC_HOST, port))
{
	init();
}

void
CIpcServer::init()
{
	m_clientsMutex = ARCH->newMutex();
	m_address.resolve();

	m_events->adoptHandler(
		m_events->forIListenSocket().connecting(), &m_socket,
		new TMethodEventJob<CIpcServer>(
		this, &CIpcServer::handleClientConnecting));
}

CIpcServer::~CIpcServer()
{
	ARCH->lockMutex(m_clientsMutex);
	CClientList::iterator it;
	for (it = m_clients.begin(); it != m_clients.end(); it++) {
		deleteClient(*it);
	}
	m_clients.empty();
	ARCH->unlockMutex(m_clientsMutex);
	ARCH->closeMutex(m_clientsMutex);
	
	m_events->removeHandler(m_events->forIListenSocket().connecting(), &m_socket);
}

void
CIpcServer::listen()
{
	m_socket.bind(m_address);
}

void
CIpcServer::handleClientConnecting(const CEvent&, void*)
{
	synergy::IStream* stream = m_socket.accept();
	if (stream == NULL) {
		return;
	}

	LOG((CLOG_DEBUG "accepted ipc client connection"));

	ARCH->lockMutex(m_clientsMutex);
	CIpcClientProxy* proxy = new CIpcClientProxy(*stream, m_events);
	m_clients.push_back(proxy);
	ARCH->unlockMutex(m_clientsMutex);

	m_events->adoptHandler(
		m_events->forCIpcClientProxy().disconnected(), proxy,
		new TMethodEventJob<CIpcServer>(
		this, &CIpcServer::handleClientDisconnected));

	m_events->adoptHandler(
		m_events->forCIpcClientProxy().messageReceived(), proxy,
		new TMethodEventJob<CIpcServer>(
		this, &CIpcServer::handleMessageReceived));

	m_events->addEvent(CEvent(
		m_events->forCIpcServer().clientConnected(), this, proxy, CEvent::kDontFreeData));
}

void
CIpcServer::handleClientDisconnected(const CEvent& e, void*)
{
	CIpcClientProxy* proxy = static_cast<CIpcClientProxy*>(e.getTarget());

	CArchMutexLock lock(m_clientsMutex);
	m_clients.remove(proxy);
	deleteClient(proxy);

	LOG((CLOG_DEBUG "ipc client proxy removed, connected=%d", m_clients.size()));
}

void
CIpcServer::handleMessageReceived(const CEvent& e, void*)
{
	CEvent event(m_events->forCIpcServer().messageReceived(), this);
	event.setDataObject(e.getDataObject());
	m_events->addEvent(event);
}

void
CIpcServer::deleteClient(CIpcClientProxy* proxy)
{
	m_events->removeHandler(m_events->forCIpcClientProxy().messageReceived(), proxy);
	m_events->removeHandler(m_events->forCIpcClientProxy().disconnected(), proxy);
	delete proxy;
}

bool
CIpcServer::hasClients(EIpcClientType clientType) const
{
	CArchMutexLock lock(m_clientsMutex);

	if (m_clients.empty()) {
		return false;
	}

	CClientList::const_iterator it;
	for (it = m_clients.begin(); it != m_clients.end(); it++) {
		// at least one client is alive and type matches, there are clients.
		CIpcClientProxy* p = *it;
		if (!p->m_disconnecting && p->m_clientType == clientType) {
			return true;
		}
	}

	// all clients must be disconnecting, no active clients.
	return false;
}

void
CIpcServer::send(const CIpcMessage& message, EIpcClientType filterType)
{
	CArchMutexLock lock(m_clientsMutex);

	CClientList::iterator it;
	for (it = m_clients.begin(); it != m_clients.end(); it++) {
		CIpcClientProxy* proxy = *it;
		if (proxy->m_clientType == filterType) {
			proxy->send(message);
		}
	}
}
