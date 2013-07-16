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

#pragma once

#include "CNetworkAddress.h"
#include "CTCPSocket.h"
#include "CEventTypes.h"

class CIpcServerProxy;
class CIpcMessage;
class IEventQueue;
class CSocketMultiplexer;

//! IPC client for communication between daemon and GUI.
/*!
 * See \ref CIpcServer description.
 */
class CIpcClient {
public:
	CIpcClient(IEventQueue* events, CSocketMultiplexer* socketMultiplexer);
	CIpcClient(IEventQueue* events, CSocketMultiplexer* socketMultiplexer, int port);
	virtual ~CIpcClient();

	//! @name manipulators
	//@{

	//! Connects to the IPC server at localhost.
	void				connect();
	
	//! Disconnects from the IPC server.
	void				disconnect();

	//! Sends a message to the server.
	void				send(const CIpcMessage& message);

	//@}

private:
	void				init();
	void				handleConnected(const CEvent&, void*);
	void				handleMessageReceived(const CEvent&, void*);

private:
	CNetworkAddress		m_serverAddress;
	CTCPSocket			m_socket;
	CIpcServerProxy*	m_server;
	IEventQueue*		m_events;
};
