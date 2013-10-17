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

#include "CIpcClient.h"
#include "Ipc.h"
#include "CIpcServerProxy.h"
#include "TMethodEventJob.h"
#include "CIpcMessage.h"

//
// CIpcClient
//

CIpcClient::CIpcClient(IEventQueue* events, CSocketMultiplexer* socketMultiplexer) :
	m_serverAddress(CNetworkAddress(IPC_HOST, IPC_PORT)),
	m_server(nullptr),
	m_socket(events, socketMultiplexer),
	m_events(events)
{
	init();
}

CIpcClient::CIpcClient(IEventQueue* events, CSocketMultiplexer* socketMultiplexer, int port) :
	m_serverAddress(CNetworkAddress(IPC_HOST, port)),
	m_server(nullptr),
	m_socket(events, socketMultiplexer),
	m_events(events)
{
	init();
}

void
CIpcClient::init()
{
	m_serverAddress.resolve();
}

CIpcClient::~CIpcClient()
{
}

void
CIpcClient::connect()
{
	m_events->adoptHandler(
		m_events->forIDataSocket().connected(), m_socket.getEventTarget(),
		new TMethodEventJob<CIpcClient>(
		this, &CIpcClient::handleConnected));

	m_socket.connect(m_serverAddress);
	m_server = new CIpcServerProxy(m_socket, m_events);

	m_events->adoptHandler(
		m_events->forCIpcServerProxy().messageReceived(), m_server,
		new TMethodEventJob<CIpcClient>(
		this, &CIpcClient::handleMessageReceived));
}

void
CIpcClient::disconnect()
{
	m_events->removeHandler(m_events->forIDataSocket().connected(), m_socket.getEventTarget());
	m_events->removeHandler(m_events->forCIpcServerProxy().messageReceived(), m_server);

	m_server->disconnect();
	delete m_server;
	m_server = nullptr;
}

void
CIpcClient::send(const CIpcMessage& message)
{
	assert(m_server != nullptr);
	m_server->send(message);
}

void
CIpcClient::handleConnected(const CEvent&, void*)
{
	m_events->addEvent(CEvent(
		m_events->forCIpcClient().connected(), this, m_server, CEvent::kDontFreeData));

	CIpcHelloMessage message(kIpcClientNode);
	send(message);
}

void
CIpcClient::handleMessageReceived(const CEvent& e, void*)
{
	CEvent event(m_events->forCIpcClient().messageReceived(), this);
	event.setDataObject(e.getDataObject());
	m_events->addEvent(event);
}
