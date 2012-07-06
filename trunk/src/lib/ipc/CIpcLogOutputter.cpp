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

#include "CIpcLogOutputter.h"
#include "CIpcServer.h"
#include "CIpcMessage.h"
#include "Ipc.h"
#include "CEvent.h"
#include "CEventQueue.h"
#include "TMethodEventJob.h"
#include "CIpcClientProxy.h"

CIpcLogOutputter::CIpcLogOutputter(CIpcServer& ipcServer) :
m_ipcServer(ipcServer)
{
}

CIpcLogOutputter::~CIpcLogOutputter()
{
}

void
CIpcLogOutputter::sendBuffer(CIpcClientProxy& proxy)
{
	while (m_buffer.size() != 0) {
		CString text = m_buffer.front();
		m_buffer.pop();
		
		CIpcMessage message;
		message.m_type = kIpcLogLine;
		message.m_data = new CString(text);
		proxy.send(message);
	}
}

void
CIpcLogOutputter::open(const char* title)
{
}

void
CIpcLogOutputter::close()
{
}

void
CIpcLogOutputter::show(bool showIfEmpty)
{
}

bool
CIpcLogOutputter::write(ELevel level, const char* text)
{
	if (m_ipcServer.hasClients(kIpcClientGui)) {
		CIpcMessage message;
		message.m_type = kIpcLogLine;
		message.m_data = new CString(text);
		m_ipcServer.send(message, kIpcClientGui);
	}
	else {
		m_buffer.push(text);
	}
	
	return true;
}
