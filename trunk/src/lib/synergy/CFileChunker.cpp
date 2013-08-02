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

#include "CFileChunker.h"
#include "BasicTypes.h"
#include "ProtocolTypes.h"
#include "CEvent.h"
#include "IEventQueue.h"
#include "CEventTypes.h"
#include "CLog.h"
#include "CStopwatch.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

#define PAUSE_TIME_HACK 0.1

using namespace std;

const size_t CFileChunker::m_chunkSize = 512 * 1024; // 512kb

void
CFileChunker::sendFileChunks(char* filename, IEventQueue* events, void* eventTarget)
{
	std::fstream file(reinterpret_cast<char*>(filename), std::ios::in | std::ios::binary);

	if (!file.is_open()) {
		throw runtime_error("failed to open file");
	}

	// check file size
	file.seekg (0, std::ios::end);
	size_t size = (size_t)file.tellg();

	// send first message (file size)
	CString fileSize = intToString(size);
	size_t sizeLength = fileSize.size();
	CFileChunk* sizeMessage = new CFileChunk(sizeLength + 2);
	char* chunkData = sizeMessage->m_chunk;

	chunkData[0] = kFileStart;
	memcpy(&chunkData[1], fileSize.c_str(), sizeLength);
	chunkData[sizeLength + 1] = '\0';
	events->addEvent(CEvent(events->forIScreen().fileChunkSending(), eventTarget, sizeMessage));

	// send chunk messages with a fixed chunk size
	size_t sentLength = 0;
	size_t chunkSize = m_chunkSize;
	CStopwatch stopwatch;
	stopwatch.start();
	file.seekg (0, std::ios::beg);
	while (true) {
		if (stopwatch.getTime() > PAUSE_TIME_HACK) {
			// make sure we don't read too much from the mock data.
			if (sentLength + chunkSize > size) {
				chunkSize = size - sentLength;
			}

			// for fileChunk->m_chunk, the first byte is the chunk mark, last is \0
			CFileChunk* fileChunk = new CFileChunk(chunkSize + 2);
			char* chunkData = fileChunk->m_chunk;

			chunkData[0] = kFileChunk;
			file.read(&chunkData[1], chunkSize);
			chunkData[chunkSize + 1] = '\0';
			events->addEvent(CEvent(events->forIScreen().fileChunkSending(), eventTarget, fileChunk));

			sentLength += chunkSize;
			file.seekg (sentLength, std::ios::beg);

			if (sentLength == size) {
				break;
			}

			stopwatch.reset();
		}
	}

	// send last message
	CFileChunk* transferFinished = new CFileChunk(2);
	chunkData = transferFinished->m_chunk;

	chunkData[0] = kFileEnd;
	chunkData[1] = '\0';
	events->addEvent(CEvent(events->forIScreen().fileChunkSending(), eventTarget, transferFinished));

	file.close();
}

CString
CFileChunker::intToString(size_t i)
{
	stringstream ss;
	ss << i;
	return ss.str();
}
