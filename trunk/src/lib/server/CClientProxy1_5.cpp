/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include "CClientProxy1_5.h"
#include "CProtocolUtil.h"
#include "CLog.h"
#include "IStream.h"
#include "CServer.h"

//
// CClientProxy1_5
//

CClientProxy1_5::CClientProxy1_5(const CString& name, synergy::IStream* stream, CServer* server, IEventQueue* events) :
	CClientProxy1_4(name, stream, server, events),
	m_events(events)
{
}

CClientProxy1_5::~CClientProxy1_5()
{
}

void
CClientProxy1_5::fileChunkSending(UInt8 mark, char* data, size_t dataSize)
{
	CString chunk(data, dataSize);

	switch (mark) {
	case kFileStart:
		LOG((CLOG_DEBUG2 "file sending start: size=%s", data));
		break;

	case kFileChunk:
		LOG((CLOG_DEBUG2 "file chunk sending: size=%i", chunk.size()));
		break;

	case kFileEnd:
		LOG((CLOG_DEBUG2 "file sending finished"));
		break;
	}

	CProtocolUtil::writef(getStream(), kMsgDFileTransfer, mark, &chunk);
}

bool
CClientProxy1_5::parseMessage(const UInt8* code)
{
	if (memcmp(code, kMsgDFileTransfer, 4) == 0) {
		fileChunkReceived();
	}
	else {
		return CClientProxy1_4::parseMessage(code);
	}

	return true;
}

void
CClientProxy1_5::fileChunkReceived()
{
	// parse
	UInt8 mark;
	CString content;
	CProtocolUtil::readf(getStream(), kMsgDFileTransfer + 4, &mark, &content);

	CServer* server = getServer();
	switch (mark) {
	case kFileStart:
		LOG((CLOG_DEBUG2 "recv file data from client: file size=%s", content.c_str()));
		server->clearReceivedFileData();
		server->setExpectedFileSize(content);
		break;

	case kFileChunk:
		LOG((CLOG_DEBUG2 "recv file data from client: chunck size=%i", content.size()));
		server->fileChunkReceived(content);
		break;

	case kFileEnd:
		LOG((CLOG_DEBUG2 "file data transfer finished"));
		m_events->addEvent(CEvent(m_events->forIScreen().fileRecieveComplete(), server));
		break;
	}
}
