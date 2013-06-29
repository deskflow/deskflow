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
#include "CEventTypes.h"

namespace synergy { class IStream; }
class CIpcMessage;
class CIpcCommandMessage;
class CIpcHelloMessage;
class IEventQueue;

class CIpcClientProxy {
	friend class CIpcServer;

public:
	CIpcClientProxy(synergy::IStream& stream, IEventQueue* events);
	virtual ~CIpcClientProxy();

private:
	void				send(const CIpcMessage& message);
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
	IEventQueue*		m_events;
};
