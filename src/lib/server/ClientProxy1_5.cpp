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

const UInt16 CClientProxy1_5::m_intervalThreshold = 1;

CClientProxy1_5::CClientProxy1_5(const CString& name, synergy::IStream* stream, CServer* server, IEventQueue* events) :
	CClientProxy1_4(name, stream, server, events),
	m_events(events),
	m_stopwatch(true),
	m_elapsedTime(0),
	m_receivedDataSize(0)
{
}

CClientProxy1_5::~CClientProxy1_5()
{
}

void
CClientProxy1_5::draggingInfoSending(UInt32 fileCount, const char* data, size_t dataSize)
{
	CString info(data, dataSize);

	CProtocolUtil::writef(getStream(), kMsgDDragInfo, fileCount, &info);
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
	else if (memcmp(code, kMsgDDragInfo, 4) == 0) {
		dragInfoReceived();
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
	UInt8 mark = 0;
	CString content;
	CProtocolUtil::readf(getStream(), kMsgDFileTransfer + 4, &mark, &content);

	CServer* server = getServer();
	switch (mark) {
	case kFileStart:
		server->clearReceivedFileData();
		server->setExpectedFileSize(content);
		if (CLOG->getFilter() >= kDEBUG2) {
			LOG((CLOG_DEBUG2 "recv file data from client: file size=%s", content.c_str()));
			m_stopwatch.start();
		}
		break;

	case kFileChunk:
		server->fileChunkReceived(content);
		if (CLOG->getFilter() >= kDEBUG2) {
				LOG((CLOG_DEBUG2 "recv file data from client: chunck size=%i", content.size()));
				double interval = m_stopwatch.getTime();
				m_receivedDataSize += content.size();
				LOG((CLOG_DEBUG2 "recv file data from client: interval=%f s", interval));
				if (interval >= m_intervalThreshold) {
					double averageSpeed = m_receivedDataSize / interval / 1000;
					LOG((CLOG_DEBUG2 "recv file data from client: average speed=%f kb/s", averageSpeed));

					m_receivedDataSize = 0;
					m_elapsedTime += interval;
					m_stopwatch.reset();
				}
			}
		break;

	case kFileEnd:
		m_events->addEvent(CEvent(m_events->forIScreen().fileRecieveCompleted(), server));
		if (CLOG->getFilter() >= kDEBUG2) {
			LOG((CLOG_DEBUG2 "file data transfer finished"));
			m_elapsedTime += m_stopwatch.getTime();
			double averageSpeed = getServer()->getExpectedFileSize() / m_elapsedTime / 1000;
			LOG((CLOG_DEBUG2 "file data transfer finished: total time consumed=%f s", m_elapsedTime));
			LOG((CLOG_DEBUG2 "file data transfer finished: total data received=%i kb", getServer()->getExpectedFileSize() / 1000));
			LOG((CLOG_DEBUG2 "file data transfer finished: total average speed=%f kb/s", averageSpeed));
		}
		break;
	}
}

void
CClientProxy1_5::dragInfoReceived()
{
	// parse
	UInt32 fileNum = 0;
	CString content;
	CProtocolUtil::readf(getStream(), kMsgDDragInfo + 4, &fileNum, &content);
	
	m_server->dragInfoReceived(fileNum, content);
}
