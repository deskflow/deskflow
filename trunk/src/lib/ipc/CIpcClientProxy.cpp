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
m_clientType(kIpcClientUnknown),
m_disconnecting(false),
m_readMutex(ARCH->newMutex()),
m_writeMutex(ARCH->newMutex())
{
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
	
	// don't delete the stream while it's being used.
	ARCH->lockMutex(m_readMutex);
	ARCH->lockMutex(m_writeMutex);
	delete &m_stream;
	ARCH->unlockMutex(m_readMutex);
	ARCH->unlockMutex(m_writeMutex);

	ARCH->closeMutex(m_readMutex);
	ARCH->closeMutex(m_writeMutex);
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
	LOG((CLOG_DEBUG "start ipc client proxy handle data"));

	// don't allow the dtor to destroy the stream while we're using it.
	CArchMutexLock lock(m_readMutex);
	
	UInt8 codeBuf[1];
	UInt32 n = m_stream.read(codeBuf, 1);
	int code = codeBuf[0];

	while (n != 0) {

		LOG((CLOG_DEBUG "ipc client proxy read: %d", code));
		
		CIpcMessage* m = nullptr;
		switch (code) {
		case kIpcHello:
			m = parseHello();
			break;

		case kIpcCommand:
			m = parseCommand();
			break;

		default:
			disconnect();
			return;
		}
		
		// don't delete with this event; the data is passed to a new event.
		CEvent e(getMessageReceivedEvent(), this, NULL, CEvent::kDontFreeData);
		e.setDataObject(m);
		EVENTQUEUE->addEvent(e);
		
		n = m_stream.read(codeBuf, 1);
		code = codeBuf[0];
	}
	
	LOG((CLOG_DEBUG "finished ipc client proxy handle data"));
}

void
CIpcClientProxy::send(const CIpcMessage& message)
{
	// don't allow other threads to write until we've finished the entire
	// message. stream write is locked, but only for that single write.
	// also, don't allow the dtor to destroy the stream while we're using it.
	CArchMutexLock lock(m_writeMutex);

	LOG((CLOG_DEBUG "ipc client proxy write: %d", message.type()));
	
	CProtocolUtil::writef(&m_stream, "%1i", message.type());

	switch (message.type()) {
	case kIpcLogLine: {
		const CIpcLogLineMessage& llm = static_cast<const CIpcLogLineMessage&>(message);
		
		CString logLine = llm.logLine();
		const char* data = logLine.c_str();
		int len = strlen(data);

		CProtocolUtil::writef(&m_stream, "%4i", len);
		m_stream.write(data, len);
		break;
	}
			
	case kIpcShutdown:
		// no data.
		break;

	default:
		LOG((CLOG_ERR "message not supported: %d", message.type()));
		break;
	}
}

CIpcHelloMessage*
CIpcClientProxy::parseHello()
{
	UInt8 buffer[1];
	m_stream.read(buffer, 1);
	m_clientType = static_cast<EIpcClientType>(buffer[0]);

	// must be deleted by event handler.
	return new CIpcHelloMessage(m_clientType);
}

CIpcCommandMessage*
CIpcClientProxy::parseCommand()
{
	int len = 0;
	CProtocolUtil::readf(&m_stream, "%2i", &len);

	char* buffer = new char[len];
	m_stream.read(buffer, len);
	CString s(buffer, len);
	delete buffer;

	// must be deleted by event handler.
	return new CIpcCommandMessage(s);
}

void
CIpcClientProxy::disconnect()
{
	LOG((CLOG_DEBUG "ipc client proxy disconnect"));
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
