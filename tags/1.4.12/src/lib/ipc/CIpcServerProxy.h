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

namespace synergy { class IStream; }
class CIpcMessage;
class CIpcLogLineMessage;

class CIpcServerProxy {
	friend class CIpcClient;

public:
	CIpcServerProxy(synergy::IStream& stream);
	virtual ~CIpcServerProxy();

private:
	void				send(const CIpcMessage& message);

	void				handleData(const CEvent&, void*);
	CIpcLogLineMessage*	parseLogLine();
	void				disconnect();

	//! Raised when the client receives a message from the server.
	static CEvent::Type	getMessageReceivedEvent();

private:
	synergy::IStream&	m_stream;

	static CEvent::Type	s_messageReceivedEvent;
};
