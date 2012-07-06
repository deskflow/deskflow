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

#include "CIpcClientProxy.h"
#include "IStream.h"
#include "TMethodEventJob.h"
#include "Ipc.h"
#include "CLog.h"
#include "CIpcMessage.h"
#include "CProtocolUtil.h"
#include "CArch.h"

CEvent::Type			CIpcClientProxy::s_messageReceivedEvent = CEvent::kUnknown;
CEvent::Type			CIpcClientProxy::s_disconnectedEvent = CEvent::kUnknown;

CIpcClientProxy::CIpcClientProxy(synergy::IStream& stream) :
m_stream(stream),
m_enableLog(false),
m_clientType(kIpcClientUnknown),
m_disconnecting(false),
m_socketBusy(false)
{
	m_mutex = ARCH->newMutex();

	EVENTQUEUE->adoptHandler(
		m_stream.getInputReadyEvent(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleData));

	EVENTQUEUE->adoptHandler(
		m_stream.getOutputErrorEvent(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleWriteError));

	EVENTQUEUE->adoptHandler(
		m_stream.getInputShutdownEvent(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleDisconnect));

	EVENTQUEUE->adoptHandler(
		m_stream.getOutputShutdownEvent(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleWriteError));
}

CIpcClientProxy::~CIpcClientProxy()
{
	EVENTQUEUE->removeHandler(
		m_stream.getInputReadyEvent(), m_stream.getEventTarget());
	EVENTQUEUE->removeHandler(
		m_stream.getOutputErrorEvent(), m_stream.getEventTarget());
	EVENTQUEUE->removeHandler(
		m_stream.getInputShutdownEvent(), m_stream.getEventTarget());
	EVENTQUEUE->removeHandler(
		m_stream.getOutputShutdownEvent(), m_stream.getEventTarget());

	delete &m_stream;
}

void
CIpcClientProxy::handleDisconnect(const CEvent&, void*)
{
	disconnect();
	LOG((CLOG_DEBUG "ipc client disconnected"));
}

void
CIpcClientProxy::handleWriteError(const CEvent&, void*)
{
	disconnect();
	LOG((CLOG_DEBUG "ipc client write error"));
}

void
CIpcClientProxy::handleData(const CEvent&, void*)
{
	// ugh, not sure if this is needed here. the write function has it to protect
	// m_socketBusy (for dropping log messages primarily). but i think i saw a
	// deadlock even after adding this mechanism. my rationale here is that i don't
	// want the read to deadlock the stream (if that's even possible).
	CArchMutexLock lock(m_mutex);

	UInt8 code[1];
	UInt32 n = m_stream.read(code, 1);
	while (n != 0) {
		UInt8 type = code[0];

		CIpcMessage* m = new CIpcMessage();
		m->m_type = type;
		m->m_source = this;

		if (m_enableLog) {
			LOG((CLOG_DEBUG "ipc client proxy read: %d", code[0]));
		}

		switch (type) {
		case kIpcHello:
			parseHello();
			break;

		case kIpcCommand:
			m->m_data = parseCommand();
			break;

		default:
			delete m;
			disconnect();
			return;
		}

		// event deletes data.
		EVENTQUEUE->addEvent(CEvent(getMessageReceivedEvent(), this, m));

		n = m_stream.read(code, 1);
	}
}

void
CIpcClientProxy::send(const CIpcMessage& message)
{
	// discard message if we're busy writing already. this is to prevent
	// deadlock, since this function can sometimes generate log messages
	// through stream usage.
	if (m_socketBusy) {
		return;
	}

	CArchMutexLock lock(m_mutex);
	m_socketBusy = true;
	try {
		if (m_enableLog) {
			LOG((CLOG_DEBUG "ipc client proxy write: %d", message.m_type));
		}

		UInt8 code[1];
		code[0] = message.m_type;
		m_stream.write(code, 1);

		switch (message.m_type) {
		case kIpcLogLine: {
			CString* s = (CString*)message.m_data;
			const char* data = s->c_str();
		
			int len = strlen(data);
			CProtocolUtil::writef(&m_stream, "%2i", len);

			m_stream.write(data, len);
			break;
		}
			
		case kIpcShutdown:
			// no data.
			break;

		default:
			if (m_enableLog) {
				LOG((CLOG_ERR "message not supported: %d", message.m_type));
			}
			break;
		}
		
		m_socketBusy = false;
	}
	catch (...) {
		m_socketBusy = false;
		throw;
	}
}

void
CIpcClientProxy::parseHello()
{
	UInt8 buffer[1];
	m_stream.read(buffer, 1);
	m_clientType = static_cast<EIpcClientType>(buffer[0]);
}

void*
CIpcClientProxy::parseCommand()
{
	int len = 0;
	CProtocolUtil::readf(&m_stream, "%2i", &len);

	UInt8* buffer = new UInt8[len];
	m_stream.read(buffer, len);

	// delete by event cleanup.
	return new CString((const char*)buffer, len);
}

void
CIpcClientProxy::disconnect()
{
	m_disconnecting = true;
	EVENTQUEUE->addEvent(CEvent(getDisconnectedEvent(), this));
}

CEvent::Type
CIpcClientProxy::getMessageReceivedEvent()
{
	return EVENTQUEUE->registerTypeOnce(
		s_messageReceivedEvent, "CIpcClientProxy::messageReceived");
}

CEvent::Type
CIpcClientProxy::getDisconnectedEvent()
{
	return EVENTQUEUE->registerTypeOnce(
		s_disconnectedEvent, "CIpcClientProxy::disconnected");
}
