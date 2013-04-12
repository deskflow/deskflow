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

#include "CEvent.h"
#include "Ipc.h"
#include "IArchMultithread.h"

namespace synergy { class IStream; }
class CIpcMessage;
class CIpcCommandMessage;
class CIpcHelloMessage;

class CIpcClientProxy {
	friend class CIpcServer;

public:
	CIpcClientProxy(synergy::IStream& stream);
	virtual ~CIpcClientProxy();

private:
	//! Send a message to the client.
	void				send(const CIpcMessage& message);

	//! Raised when the server receives a message from a client.
	static CEvent::Type	getMessageReceivedEvent();

	//! Raised when the client disconnects from the server.
	static CEvent::Type	getDisconnectedEvent();

	void				handleData(const CEvent&, void*);
	void				handleDisconnect(const CEvent&, void*);
	void				handleWriteError(const CEvent&, void*);
	CIpcHelloMessage*	parseHello();
	CIpcCommandMessage*	parseCommand();
	void				disconnect();
	
private:
	synergy::IStream&	m_stream;
	EIpcClientType		m_clientType;
	bool				m_disconnecting;
	CArchMutex			m_readMutex;
	CArchMutex			m_writeMutex;

	static CEvent::Type	s_messageReceivedEvent;
	static CEvent::Type	s_disconnectedEvent;
};
