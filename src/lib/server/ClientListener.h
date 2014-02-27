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

#ifndef CCLIENTLISTENER_H
#define CCLIENTLISTENER_H

#include "CConfig.h"
#include "CEvent.h"
#include "stddeque.h"
#include "stdset.h"
#include "CCryptoOptions.h"
#include "CEventTypes.h"

class CClientProxy;
class CClientProxyUnknown;
class CNetworkAddress;
class IListenSocket;
class ISocketFactory;
class IStreamFilterFactory;
class CServer;
class IEventQueue;

class CClientListener {
public:
	// The factories are adopted.
	CClientListener(const CNetworkAddress&,
							ISocketFactory*,
							IStreamFilterFactory*,
							const CCryptoOptions& crypto,
							IEventQueue* events);
	~CClientListener();

	//! @name manipulators
	//@{

	void				setServer(CServer* server);

	//@}

	//! @name accessors
	//@{

	//! Get next connected client
	/*!
	Returns the next connected client and removes it from the internal
	list.  The client is responsible for deleting the returned client.
	Returns NULL if no clients are available.
	*/
	CClientProxy*		getNextClient();

	//! Get server which owns this listener
	CServer*			getServer() { return m_server; }

	//@}

private:
	// client connection event handlers
	void				handleClientConnecting(const CEvent&, void*);
	void				handleUnknownClient(const CEvent&, void*);
	void				handleClientDisconnected(const CEvent&, void*);

private:
	typedef std::set<CClientProxyUnknown*> CNewClients;
	typedef std::deque<CClientProxy*> CWaitingClients;

	IListenSocket*		m_listen;
	ISocketFactory*		m_socketFactory;
	IStreamFilterFactory*	m_streamFilterFactory;
	CNewClients			m_newClients;
	CWaitingClients		m_waitingClients;
	CServer*			m_server;
	CCryptoOptions		m_crypto;
	IEventQueue*		m_events;
};

#endif
