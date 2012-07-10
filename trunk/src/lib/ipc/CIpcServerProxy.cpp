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

#include "CIpcServerProxy.h"
#include "IStream.h"
#include "TMethodEventJob.h"
#include "CLog.h"
#include "CIpcMessage.h"
#include "Ipc.h"
#include "CProtocolUtil.h"

CEvent::Type			CIpcServerProxy::s_messageReceivedEvent = CEvent::kUnknown;

CIpcServerProxy::CIpcServerProxy(synergy::IStream& stream) :
m_stream(stream)
{
	EVENTQUEUE->adoptHandler(m_stream.getInputReadyEvent(),
		stream.getEventTarget(),
		new TMethodEventJob<CIpcServerProxy>(
		this, &CIpcServerProxy::handleData));
}

CIpcServerProxy::~CIpcServerProxy()
{
	EVENTQUEUE->removeHandler(m_stream.getInputReadyEvent(),
		m_stream.getEventTarget());

	m_stream.close();
}

void
CIpcServerProxy::handleData(const CEvent&, void*)
{
	LOG((CLOG_DEBUG "start ipc server proxy handle data"));

	UInt8 codeBuf[1];
	UInt32 n = m_stream.read(codeBuf, 1);
	int code = codeBuf[0];

	while (n != 0) {

		LOG((CLOG_DEBUG "ipc server proxy read: %d", code));
		
		CIpcMessage* m = nullptr;
		switch (code) {
		case kIpcLogLine:
			m = parseLogLine();
			break;
			
		case kIpcShutdown:
			m = new CIpcShutdownMessage();
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
	
	LOG((CLOG_DEBUG "finished ipc server proxy handle data"));
}

void
CIpcServerProxy::send(const CIpcMessage& message)
{
	LOG((CLOG_DEBUG "ipc server proxy write: %d", message.type()));

	CProtocolUtil::writef(&m_stream, "%1i", message.type());

	switch (message.type()) {
	case kIpcHello: {
		const CIpcHelloMessage& hm = static_cast<const CIpcHelloMessage&>(message);
		CProtocolUtil::writef(&m_stream, "%1i", hm.clientType());
		break;
	}

	case kIpcCommand: {
		const CIpcCommandMessage& cm = static_cast<const CIpcCommandMessage&>(message);
		
		CString command = cm.command();
		const char* data = command.c_str();
		int len = strlen(data);

		CProtocolUtil::writef(&m_stream, "%2i", len);
		m_stream.write(data, len);
		break;
	}

	default:
		LOG((CLOG_ERR "message not supported: %d", message.type()));
		break;
	}
}

CIpcLogLineMessage*
CIpcServerProxy::parseLogLine()
{
	int len = 0;
	CProtocolUtil::readf(&m_stream, "%4i", &len);

	char* buffer = new char[len];
	m_stream.read(buffer, len);
	CString s(buffer, len);
	delete buffer;
	
	// must be deleted by event handler.
	return new CIpcLogLineMessage(s);
}

void
CIpcServerProxy::disconnect()
{
	LOG((CLOG_NOTE "disconnect, closing stream"));
	m_stream.close();
}

CEvent::Type
CIpcServerProxy::getMessageReceivedEvent()
{
	return EVENTQUEUE->registerTypeOnce(
		s_messageReceivedEvent, "CIpcServerProxy::messageReceived");
}
