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

#include "CIpcClientProxy.h"
#include "IStream.h"
#include "TMethodEventJob.h"
#include "Ipc.h"
#include "CLog.h"
#include "CIpcMessage.h"
#include "CProtocolUtil.h"
#include "CArch.h"

//
// CIpcClientProxy
//

CIpcClientProxy::CIpcClientProxy(synergy::IStream& stream, IEventQueue* events) :
	m_stream(stream),
	m_clientType(kIpcClientUnknown),
	m_disconnecting(false),
	m_readMutex(ARCH->newMutex()),
	m_writeMutex(ARCH->newMutex()),
	m_events(events)
{
	m_events->adoptHandler(
		m_events->forIStream().inputReady(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleData));

	m_events->adoptHandler(
		m_events->forIStream().outputError(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleWriteError));

	m_events->adoptHandler(
		m_events->forIStream().inputShutdown(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleDisconnect));

	m_events->adoptHandler(
		m_events->forIStream().outputShutdown(), stream.getEventTarget(),
		new TMethodEventJob<CIpcClientProxy>(
		this, &CIpcClientProxy::handleWriteError));
}

CIpcClientProxy::~CIpcClientProxy()
{
	m_events->removeHandler(
		m_events->forIStream().inputReady(), m_stream.getEventTarget());
	m_events->removeHandler(
		m_events->forIStream().outputError(), m_stream.getEventTarget());
	m_events->removeHandler(
		m_events->forIStream().inputShutdown(), m_stream.getEventTarget());
	m_events->removeHandler(
		m_events->forIStream().outputShutdown(), m_stream.getEventTarget());
	
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
	// don't allow the dtor to destroy the stream while we're using it.
	CArchMutexLock lock(m_readMutex);

	LOG((CLOG_DEBUG "start ipc handle data"));

	UInt8 code[4];
	UInt32 n = m_stream.read(code, 4);
	while (n != 0) {

		LOG((CLOG_DEBUG "ipc read: %c%c%c%c",
			code[0], code[1], code[2], code[3]));

		CIpcMessage* m = nullptr;
		if (memcmp(code, kIpcMsgHello, 4) == 0) {
			m = parseHello();
		}
		else if (memcmp(code, kIpcMsgCommand, 4) == 0) {
			m = parseCommand();
		}
		else {
			LOG((CLOG_ERR "invalid ipc message"));
			disconnect();
		}

		// don't delete with this event; the data is passed to a new event.
		CEvent e(m_events->forCIpcClientProxy().messageReceived(), this, NULL, CEvent::kDontFreeData);
		e.setDataObject(m);
		m_events->addEvent(e);

		n = m_stream.read(code, 4);
	}

	LOG((CLOG_DEBUG "finished ipc handle data"));
}

void
CIpcClientProxy::send(const CIpcMessage& message)
{
	// don't allow other threads to write until we've finished the entire
	// message. stream write is locked, but only for that single write.
	// also, don't allow the dtor to destroy the stream while we're using it.
	CArchMutexLock lock(m_writeMutex);

	LOG((CLOG_DEBUG "ipc write: %d", message.type()));

	switch (message.type()) {
	case kIpcLogLine: {
		const CIpcLogLineMessage& llm = static_cast<const CIpcLogLineMessage&>(message);
		CString logLine = llm.logLine();
		CProtocolUtil::writef(&m_stream, kIpcMsgLogLine, &logLine);
		break;
	}
			
	case kIpcShutdown:
		CProtocolUtil::writef(&m_stream, kIpcMsgShutdown);
		break;

	default:
		LOG((CLOG_ERR "ipc message not supported: %d", message.type()));
		break;
	}
}

CIpcHelloMessage*
CIpcClientProxy::parseHello()
{
	UInt8 type;
	CProtocolUtil::readf(&m_stream, kIpcMsgHello + 4, &type);

	m_clientType = static_cast<EIpcClientType>(type);

	// must be deleted by event handler.
	return new CIpcHelloMessage(m_clientType);
}

CIpcCommandMessage*
CIpcClientProxy::parseCommand()
{
	CString command;
	UInt8 elevate;
	CProtocolUtil::readf(&m_stream, kIpcMsgCommand + 4, &command, &elevate);

	// must be deleted by event handler.
	return new CIpcCommandMessage(command, elevate != 0);
}

void
CIpcClientProxy::disconnect()
{
	LOG((CLOG_DEBUG "ipc disconnect, closing stream"));
	m_disconnecting = true;
	m_stream.close();
	m_events->addEvent(CEvent(m_events->forCIpcClientProxy().disconnected(), this));
}
