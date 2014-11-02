/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "net/ISocketFactory.h"

class IEventQueue;
class SocketMultiplexer;

//! Socket factory for TCP sockets
class CTCPSocketFactory : public ISocketFactory {
public:
	CTCPSocketFactory(IEventQueue* events, SocketMultiplexer* socketMultiplexer);
	virtual ~CTCPSocketFactory();

	// ISocketFactory overrides
	virtual IDataSocket*	create() const;
	virtual IListenSocket*	createListen() const;

private:
	IEventQueue*		m_events;
	SocketMultiplexer* m_socketMultiplexer;
};
