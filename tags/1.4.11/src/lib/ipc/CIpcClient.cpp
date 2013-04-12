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

CEvent::Type			CIpcClient::s_connectedEvent = CEvent::kUnknown;
CEvent::Type			CIpcClient::s_messageReceivedEvent = CEvent::kUnknown;

CIpcClient::CIpcClient() :
m_serverAddress(CNetworkAddress(IPC_HOST, IPC_PORT)),
m_server(nullptr)
{
	init();
}

CIpcClient::CIpcClient(int port) :
m_serverAddress(CNetworkAddress(IPC_HOST, port)),
m_server(nullptr)
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
	EVENTQUEUE->adoptHandler(
		IDataSocket::getConnectedEvent(), m_socket.getEventTarget(),
		new TMethodEventJob<CIpcClient>(
		this, &CIpcClient::handleConnected));

	m_socket.connect(m_serverAddress);
	m_server = new CIpcServerProxy(m_socket);

	EVENTQUEUE->adoptHandler(
		CIpcServerProxy::getMessageReceivedEvent(), m_server,
		new TMethodEventJob<CIpcClient>(
		this, &CIpcClient::handleMessageReceived));
}

void
CIpcClient::disconnect()
{
	EVENTQUEUE->removeHandler(IDataSocket::getConnectedEvent(), m_socket.getEventTarget());
	EVENTQUEUE->removeHandler(CIpcServerProxy::getMessageReceivedEvent(), m_server);

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

CEvent::Type
CIpcClient::getConnectedEvent()
{
	return EVENTQUEUE->registerTypeOnce(
		s_connectedEvent, "CIpcClient::connected");
}

CEvent::Type
CIpcClient::getMessageReceivedEvent()
{
	return EVENTQUEUE->registerTypeOnce(
		s_messageReceivedEvent, "CIpcClient::messageReceived");
}

void
CIpcClient::handleConnected(const CEvent&, void*)
{
	EVENTQUEUE->addEvent(CEvent(
		getConnectedEvent(), this, m_server, CEvent::kDontFreeData));

	CIpcHelloMessage message(kIpcClientNode);
	send(message);
}

void
CIpcClient::handleMessageReceived(const CEvent& e, void*)
{
	CEvent event(getMessageReceivedEvent(), this);
	event.setDataObject(e.getDataObject());
	EVENTQUEUE->addEvent(event);
}
