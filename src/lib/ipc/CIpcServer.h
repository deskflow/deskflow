/*
 * synergy -- mouse and keyboard sharing utility
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

#include "CTCPListenSocket.h"
#include "CNetworkAddress.h"
#include <set>

class CEvent;
class CIpcClientProxy;

//! IPC server for communication between daemon and GUI.
/*!
The IPC server listens on localhost. The IPC client runs on both the
client/server process or the GUI. The IPC server runs on the daemon process.
This allows the GUI to send config changes to the daemon and client/server,
and allows the daemon and client/server to send log data to the GUI.
*/
class CIpcServer {
public:
	CIpcServer();
	virtual ~CIpcServer();

	//! @name manipulators
	//@{

	//! Opens a TCP socket only allowing local connections.
	void				listen();

	//@}
	//! @name accessors
	//@{

	//! This event is raised when we have created the client proxy.
	static CEvent::Type		getClientConnectedEvent();

	//@}

private:
	void				handleClientConnecting(const CEvent&, void*);

private:
	typedef std::set<CIpcClientProxy*> CClientSet;

	CTCPListenSocket	m_socket;
	CNetworkAddress		m_address;
	CClientSet			m_clients;

	static CEvent::Type	s_clientConnectedEvent;
};
