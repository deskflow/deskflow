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

#pragma once

#include "server/Config.h"
#include "io/CryptoOptions.h"
#include "base/EventTypes.h"
#include "base/Event.h"
#include "common/stddeque.h"
#include "common/stdset.h"

class ClientProxy;
class ClientProxyUnknown;
class NetworkAddress;
class IListenSocket;
class ISocketFactory;
class IStreamFilterFactory;
class Server;
class IEventQueue;

class ClientListener {
public:
	// The factories are adopted.
	ClientListener(const NetworkAddress&,
							ISocketFactory*,
							IStreamFilterFactory*,
							const CryptoOptions& crypto,
							IEventQueue* events);
	~ClientListener();

	//! @name manipulators
	//@{

	void				setServer(Server* server);

	//@}

	//! @name accessors
	//@{

	//! Get next connected client
	/*!
	Returns the next connected client and removes it from the internal
	list.  The client is responsible for deleting the returned client.
	Returns NULL if no clients are available.
	*/
	ClientProxy*		getNextClient();

	//! Get server which owns this listener
	Server*			getServer() { return m_server; }

	//@}

private:
	// client connection event handlers
	void				handleClientConnecting(const Event&, void*);
	void				handleUnknownClient(const Event&, void*);
	void				handleClientDisconnected(const Event&, void*);

private:
	typedef std::set<ClientProxyUnknown*> NewClients;
	typedef std::deque<ClientProxy*> WaitingClients;

	IListenSocket*		m_listen;
	ISocketFactory*		m_socketFactory;
	IStreamFilterFactory*	m_streamFilterFactory;
	NewClients			m_newClients;
	WaitingClients		m_waitingClients;
	Server*			m_server;
	CryptoOptions		m_crypto;
	IEventQueue*		m_events;
};
