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

#ifndef CCLIENTPROXYUNKNOWN_H
#define CCLIENTPROXYUNKNOWN_H

#include "CEvent.h"
#include "CEventTypes.h"

class CClientProxy;
class CEventQueueTimer;
namespace synergy { class IStream; }
class CServer;
class IEventQueue;

class CClientProxyUnknown {
public:
	CClientProxyUnknown(synergy::IStream* stream, double timeout, CServer* server, IEventQueue* events);
	~CClientProxyUnknown();

	//! @name manipulators
	//@{

	//! Get the client proxy
	/*!
	Returns the client proxy created after a successful handshake
	(i.e. when this object sends a success event).  Returns NULL
	if the handshake is unsuccessful or incomplete.
	*/
	CClientProxy*		orphanClientProxy();

	//@}

private:
	void				sendSuccess();
	void				sendFailure();
	void				addStreamHandlers();
	void				addProxyHandlers();
	void				removeHandlers();
	void				removeTimer();
	void				handleData(const CEvent&, void*);
	void				handleWriteError(const CEvent&, void*);
	void				handleTimeout(const CEvent&, void*);
	void				handleDisconnect(const CEvent&, void*);
	void				handleReady(const CEvent&, void*);

private:
	synergy::IStream*	m_stream;
	CEventQueueTimer*	m_timer;
	CClientProxy*		m_proxy;
	bool				m_ready;
	CServer*			m_server;
	IEventQueue*		m_events;
};

#endif
