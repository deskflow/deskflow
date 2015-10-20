/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "server/Config.h"
#include "base/EventTypes.h"
#include "base/Event.h"
#include "common/stddeque.h"
#include "common/stdset.h"

class ClientProxy;
class ClientProxyUnknown;
class NetworkAddress;
class IListenSocket;
class ISocketFactory;
class Server;
class IEventQueue;

class ClientListener {
public:
	// The factories are adopted.
	ClientListener(const NetworkAddress&,
							ISocketFactory*,
							IEventQueue* events,
							bool enableCrypto);
	~ClientListener();

	//! @name manipulators
	//@{

	void				setServer(Server* server);

	//@}

	void				deleteSocket(void* socket);

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
	Server*				getServer() { return m_server; }

	//@}

private:
	// client connection event handlers
	void				handleClientConnecting(const Event&, void*);
	void				handleUnknownClient(const Event&, void*);
	void				handleClientDisconnected(const Event&, void*);

	void				cleanupListenSocket();

private:
	typedef std::set<ClientProxyUnknown*> NewClients;
	typedef std::deque<ClientProxy*> WaitingClients;

	IListenSocket*		m_listen;
	ISocketFactory*		m_socketFactory;
	NewClients			m_newClients;
	WaitingClients		m_waitingClients;
	Server*				m_server;
	IEventQueue*		m_events;
	bool				m_useSecureNetwork;
};
