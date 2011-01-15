/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

class CClientProxy;
class CEventQueueTimer;
class IStream;

class CClientProxyUnknown {
public:
	CClientProxyUnknown(IStream* stream, double timeout);
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
	//! @name accessors
	//@{

	//! Get success event type
	/*!
	Returns the success event type.  This is sent when the client has
	correctly responded to the hello message.  The target is this.
	*/
	static CEvent::Type	getSuccessEvent();

	//! Get failure event type
	/*!
	Returns the failure event type.  This is sent when a client fails
	to correctly respond to the hello message.  The target is this.
	*/
	static CEvent::Type	getFailureEvent();

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
	IStream*			m_stream;
	CEventQueueTimer*	m_timer;
	CClientProxy*		m_proxy;
	bool				m_ready;

	static CEvent::Type	s_successEvent;
	static CEvent::Type	s_failureEvent;
};

#endif
