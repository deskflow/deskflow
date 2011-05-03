/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2006 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CCLIENTPROXY1_3_H
#define CCLIENTPROXY1_3_H

#include "CClientProxy1_2.h"

//! Proxy for client implementing protocol version 1.3
class CClientProxy1_3 : public CClientProxy1_2 {
public:
	CClientProxy1_3(const CString& name, IStream* adoptedStream);
	~CClientProxy1_3();

	// IClient overrides
	virtual void		mouseWheel(SInt32 xDelta, SInt32 yDelta);

protected:
	// CClientProxy overrides
	virtual bool		parseMessage(const UInt8* code);
	virtual void		resetHeartbeatRate();
	virtual void		setHeartbeatRate(double rate, double alarm);
	virtual void		resetHeartbeatTimer();
	virtual void		addHeartbeatTimer();
	virtual void		removeHeartbeatTimer();

private:
	void				handleKeepAlive(const CEvent&, void*);


private:
	double				m_keepAliveRate;
	CEventQueueTimer*	m_keepAliveTimer;
};

#endif
