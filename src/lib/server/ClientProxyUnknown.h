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

#include "base/Event.h"
#include "base/EventTypes.h"

class ClientProxy;
class EventQueueTimer;
namespace synergy { class IStream; }
class Server;
class IEventQueue;

class ClientProxyUnknown {
public:
	ClientProxyUnknown(synergy::IStream* stream, double timeout, Server* server, IEventQueue* events);
	~ClientProxyUnknown();

	//! @name manipulators
	//@{

	//! Get the client proxy
	/*!
	Returns the client proxy created after a successful handshake
	(i.e. when this object sends a success event).  Returns NULL
	if the handshake is unsuccessful or incomplete.
	*/
	ClientProxy*		orphanClientProxy();

	//! Get the stream
	synergy::IStream*	getStream() { return m_stream; }

	//@}

private:
	void				sendSuccess();
	void				sendFailure();
	void				addStreamHandlers();
	void				addProxyHandlers();
	void				removeHandlers();
	void				removeTimer();
	void				handleData(const Event&, void*);
	void				handleWriteError(const Event&, void*);
	void				handleTimeout(const Event&, void*);
	void				handleDisconnect(const Event&, void*);
	void				handleReady(const Event&, void*);

private:
	synergy::IStream*	m_stream;
	EventQueueTimer*	m_timer;
	ClientProxy*		m_proxy;
	bool				m_ready;
	Server*				m_server;
	IEventQueue*		m_events;
};
