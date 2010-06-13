/*
 * synergy -- mouse and keyboard sharing utility
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
 */

#ifndef CCLIENTLISTENER_H
#define CCLIENTLISTENER_H

#include "CConfig.h"
#include "CEvent.h"
#include "stddeque.h"
#include "stdset.h"

class CClientProxy;
class CClientProxyUnknown;
class CNetworkAddress;
class IListenSocket;
class ISocketFactory;
class IStreamFilterFactory;

class CClientListener {
public:
	// The factories are adopted.
	CClientListener(const CNetworkAddress&,
							ISocketFactory*, IStreamFilterFactory*);
	~CClientListener();

	//! @name accessors
	//@{

	//! Get next connected client
	/*!
	Returns the next connected client and removes it from the internal
	list.  The client is responsible for deleting the returned client.
	Returns NULL if no clients are available.
	*/
	CClientProxy*		getNextClient();

	//! Get connected event type
	/*!
	Returns the connected event type.  This is sent whenever a
	a client connects.
	*/
	static CEvent::Type	getConnectedEvent();

	//@}

private:
	// client connection event handlers
	void				handleClientConnecting(const CEvent&, void*);
	void				handleUnknownClient(const CEvent&, void*);
	void				handleClientDisconnected(const CEvent&, void*);

private:
	typedef std::set<CClientProxyUnknown*> CNewClients;
	typedef std::deque<CClientProxy*> CWaitingClients;

	IListenSocket*			m_listen;
	ISocketFactory*			m_socketFactory;
	IStreamFilterFactory*	m_streamFilterFactory;
	CNewClients				m_newClients;
	CWaitingClients			m_waitingClients;

	static CEvent::Type		s_connectedEvent;
};

#endif
