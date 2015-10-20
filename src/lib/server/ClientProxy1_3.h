/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2006 Chris Schoeneman
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

#include "server/ClientProxy1_2.h"

//! Proxy for client implementing protocol version 1.3
class ClientProxy1_3 : public ClientProxy1_2 {
public:
	ClientProxy1_3(const String& name, synergy::IStream* adoptedStream, IEventQueue* events);
	~ClientProxy1_3();

	// IClient overrides
	virtual void		mouseWheel(SInt32 xDelta, SInt32 yDelta);

	void				handleKeepAlive(const Event&, void*);

protected:
	// ClientProxy overrides
	virtual bool		parseMessage(const UInt8* code);
	virtual void		resetHeartbeatRate();
	virtual void		setHeartbeatRate(double rate, double alarm);
	virtual void		resetHeartbeatTimer();
	virtual void		addHeartbeatTimer();
	virtual void		removeHeartbeatTimer();
	virtual void		keepAlive();

private:
	double				m_keepAliveRate;
	EventQueueTimer*	m_keepAliveTimer;
	IEventQueue*		m_events;
};
