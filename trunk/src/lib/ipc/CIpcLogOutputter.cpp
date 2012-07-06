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
#include "CArch.h"

CIpcLogOutputter::CIpcLogOutputter(CIpcServer& ipcServer) :
m_ipcServer(ipcServer),
m_sending(false)
{
	m_mutex = ARCH->newMutex();
}

CIpcLogOutputter::~CIpcLogOutputter()
{
}

void
CIpcLogOutputter::sendBuffer(CIpcClientProxy& proxy)
{
	// drop messages logged while sending over ipc, since ipc can cause
	// log messages (sending these could cause recursion or deadlocks).
	// this has the side effect of dropping messages from other threads
	// which weren't caused by ipc, but that is just the downside of
	// logging this way.
	if (m_sending) {
		return;
	}

	CArchMutexLock lock(m_mutex);
	m_sending = true;
	try {
		while (m_buffer.size() != 0) {
			CString text = m_buffer.front();
			m_buffer.pop();
		
			CIpcMessage message;
			message.m_type = kIpcLogLine;
			message.m_data = new CString(text);
			proxy.send(message);
		}
		m_sending = false;
	}
	catch (...) {
		m_sending = false;
		throw;
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
	// drop messages logged while sending over ipc, since ipc can cause
	// log messages (sending these could cause recursion or deadlocks).
	// this has the side effect of dropping messages from other threads
	// which weren't caused by ipc, but that is just the downside of
	// logging this way.
	if (m_sending) {
		return false;
	}

	CArchMutexLock lock(m_mutex);
	m_sending = true;
	
	try {
		if (m_ipcServer.hasClients(kIpcClientGui)) {
			CIpcMessage message;
			message.m_type = kIpcLogLine;
			message.m_data = new CString(text);
			m_ipcServer.send(message, kIpcClientGui);
		}
		else {
			m_buffer.push(text);
		}
		m_sending = false;
		return true;
	}
	catch (...) {
		m_sending = false;
		throw;
	}
}
