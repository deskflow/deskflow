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

CIpcServerProxy::CIpcServerProxy(IStream& stream) :
m_stream(stream)
{
	EVENTQUEUE->adoptHandler(m_stream.getInputReadyEvent(),
		stream.getEventTarget(),
		new TMethodEventJob<CIpcServerProxy>(
		this, &CIpcServerProxy::handleData, nullptr));
}

CIpcServerProxy::~CIpcServerProxy()
{
	EVENTQUEUE->removeHandler(m_stream.getInputReadyEvent(),
		m_stream.getEventTarget());
}

void
CIpcServerProxy::handleData(const CEvent&, void*)
{
	UInt8 code[1];
	UInt32 n = m_stream.read(code, 1);
	while (n != 0) {

		CIpcMessage* m = new CIpcMessage();
		m->m_type = code[1];

		LOG((CLOG_DEBUG "ipc server proxy read: %d", code[0]));
		switch (code[0]) {
		case kIpcLogLine:
			m->m_data = parseLogLine();
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
CIpcServerProxy::send(const CIpcMessage& message)
{
	LOG((CLOG_DEBUG "ipc server proxy write: %d", message.m_type));

	UInt8 code[1];
	code[0] = message.m_type;
	m_stream.write(code, 1);

	switch (message.m_type) {
	case kIpcCommand: {
			CString* s = (CString*)message.m_data;
			const char* data = s->c_str();
			
			int len = strlen(data);
			CProtocolUtil::writef(&m_stream, "%2i", len);

			m_stream.write(data, len);
		}
		break;

	default:
		LOG((CLOG_ERR "message not supported: %d", message.m_type));
		break;
	}
}

void*
CIpcServerProxy::parseLogLine()
{
	int len = 0;
	CProtocolUtil::readf(&m_stream, "%2i", &len);

	UInt8* buffer = new UInt8[len];
	m_stream.read(buffer, len);

	return new CString((const char*)buffer, len);
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
