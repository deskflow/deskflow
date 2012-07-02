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

#include "CEvent.h"

class IStream;
class CIpcMessage;

class CIpcClientProxy {
public:
	CIpcClientProxy(IStream& stream);
	virtual ~CIpcClientProxy();

	//! Send a message to the client.
	void					send(const CIpcMessage& message);

	//! Raised when the server receives a message from a client.
	static CEvent::Type		getMessageReceivedEvent();

private:
	void				handleData(const CEvent&, void*);
	void*				parseCommand();
	void				disconnect();

public:
	IStream&			m_stream;
	
private:
	static CEvent::Type	s_messageReceivedEvent;
};
