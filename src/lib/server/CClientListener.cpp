/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "CClientListener.h"
#include "CClientProxy.h"
#include "CClientProxyUnknown.h"
#include "CPacketStreamFilter.h"
#include "IStreamFilterFactory.h"
#include "IDataSocket.h"
#include "IListenSocket.h"
#include "ISocketFactory.h"
#include "XSocket.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "CCryptoStream.h"
#include "CCryptoOptions.h"

// TODO: these are just for testing -- make sure they're gone by release!
const byte g_key[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const byte g_iv[] = "aaaaaaaaaaaaaaa";

//
// CClientListener
//

CEvent::Type			CClientListener::s_connectedEvent = CEvent::kUnknown;

CClientListener::CClientListener(const CNetworkAddress& address,
				ISocketFactory* socketFactory,
				IStreamFilterFactory* streamFilterFactory,
				const CCryptoOptions& crypto) :
	m_socketFactory(socketFactory),
	m_streamFilterFactory(streamFilterFactory),
	m_server(NULL),
	m_crypto(crypto)
{
	assert(m_socketFactory != NULL);

	try {
		// create listen socket
		m_listen = m_socketFactory->createListen();

		// bind listen address
		LOG((CLOG_DEBUG1 "binding listen socket"));
		m_listen->bind(address);
	}
	catch (XSocketAddressInUse&) {
		delete m_listen;
		delete m_socketFactory;
		delete m_streamFilterFactory;
		throw;
	}
	catch (XBase&) {
		delete m_listen;
		delete m_socketFactory;
		delete m_streamFilterFactory;
		throw;
	}
	LOG((CLOG_DEBUG1 "listening for clients"));

	// setup event handler
	EVENTQUEUE->adoptHandler(m_listen->getConnectingEvent(), m_listen,
							new TMethodEventJob<CClientListener>(this,
								&CClientListener::handleClientConnecting));
}

CClientListener::~CClientListener()
{
	LOG((CLOG_DEBUG1 "stop listening for clients"));

	// discard already connected clients
	for (CNewClients::iterator index = m_newClients.begin();
								index != m_newClients.end(); ++index) {
		CClientProxyUnknown* client = *index;
		EVENTQUEUE->removeHandler(
							CClientProxyUnknown::getSuccessEvent(), client);
		EVENTQUEUE->removeHandler(
							CClientProxyUnknown::getFailureEvent(), client);
		EVENTQUEUE->removeHandler(
							CClientProxy::getDisconnectedEvent(), client);
		delete client;
	}

	// discard waiting clients
	CClientProxy* client = getNextClient();
	while (client != NULL) {
		delete client;
		client = getNextClient();
	}

	EVENTQUEUE->removeHandler(m_listen->getConnectingEvent(), m_listen);
	delete m_listen;
	delete m_socketFactory;
	delete m_streamFilterFactory;
}

void
CClientListener::setServer(CServer* server)
{
	assert(server != NULL);
	m_server = server;
}

CClientProxy*
CClientListener::getNextClient()
{
	CClientProxy* client = NULL;
	if (!m_waitingClients.empty()) {
		client = m_waitingClients.front();
		m_waitingClients.pop_front();
		EVENTQUEUE->removeHandler(CClientProxy::getDisconnectedEvent(), client);
	}
	return client;
}

CEvent::Type
CClientListener::getConnectedEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_connectedEvent,
							"CClientListener::connected");
}

void
CClientListener::handleClientConnecting(const CEvent&, void*)
{
	// accept client connection
	synergy::IStream* stream = m_listen->accept();
	if (stream == NULL) {
		return;
	}
	LOG((CLOG_NOTE "accepted client connection"));

	// filter socket messages, including a packetizing filter
	if (m_streamFilterFactory != NULL) {
		stream = m_streamFilterFactory->create(stream, true);
	}
	stream = new CPacketStreamFilter(stream, true);
	
	if (m_crypto.m_mode != kDisabled) {
		CCryptoStream* cryptoStream = new CCryptoStream(
			EVENTQUEUE, stream, m_crypto, true);
		stream = cryptoStream;
	}

	assert(m_server != NULL);

	// create proxy for unknown client
	CClientProxyUnknown* client = new CClientProxyUnknown(stream, 30.0, m_server);
	m_newClients.insert(client);

	// watch for events from unknown client
	EVENTQUEUE->adoptHandler(CClientProxyUnknown::getSuccessEvent(), client,
							new TMethodEventJob<CClientListener>(this,
								&CClientListener::handleUnknownClient, client));
	EVENTQUEUE->adoptHandler(CClientProxyUnknown::getFailureEvent(), client,
							new TMethodEventJob<CClientListener>(this,
								&CClientListener::handleUnknownClient, client));
}

void
CClientListener::handleUnknownClient(const CEvent&, void* vclient)
{
	CClientProxyUnknown* unknownClient =
		reinterpret_cast<CClientProxyUnknown*>(vclient);

	// we should have the client in our new client list
	assert(m_newClients.count(unknownClient) == 1);

	// get the real client proxy and install it
	CClientProxy* client = unknownClient->orphanClientProxy();
	if (client != NULL) {
		// handshake was successful
		m_waitingClients.push_back(client);
		EVENTQUEUE->addEvent(CEvent(getConnectedEvent(), this));

		// watch for client to disconnect while it's in our queue
		EVENTQUEUE->adoptHandler(CClientProxy::getDisconnectedEvent(), client,
							new TMethodEventJob<CClientListener>(this,
								&CClientListener::handleClientDisconnected,
								client));
	}

	// now finished with unknown client
	EVENTQUEUE->removeHandler(CClientProxyUnknown::getSuccessEvent(), client);
	EVENTQUEUE->removeHandler(CClientProxyUnknown::getFailureEvent(), client);
	m_newClients.erase(unknownClient);
	delete unknownClient;
}

void
CClientListener::handleClientDisconnected(const CEvent&, void* vclient)
{
	CClientProxy* client = reinterpret_cast<CClientProxy*>(vclient);

	// find client in waiting clients queue
	for (CWaitingClients::iterator i = m_waitingClients.begin(),
							n = m_waitingClients.end(); i != n; ++i) {
		if (*i == client) {
			m_waitingClients.erase(i);
			EVENTQUEUE->removeHandler(CClientProxy::getDisconnectedEvent(),
							client);
			delete client;
			break;
		}
	}
}
