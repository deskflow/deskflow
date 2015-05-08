/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/FileChunker.h"

#include "synergy/protocol_types.h"
#include "base/EventTypes.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/EventTypes.h"
#include "base/Log.h"
#include "base/Stopwatch.h"
#include "common/stdexcept.h"

#include <fstream>
#include <sstream>

#define PAUSE_TIME_HACK 0.1

using namespace std;

const size_t FileChunker::m_chunkSize = 512 * 1024; // 512kb

void
FileChunker::sendFileChunks(char* filename, IEventQueue* events, void* eventTarget)
{
	std::fstream file(reinterpret_cast<char*>(filename), std::ios::in | std::ios::binary);

	if (!file.is_open()) {
		throw runtime_error("failed to open file");
	}

	// check file size
	file.seekg (0, std::ios::end);
	size_t size = (size_t)file.tellg();

	// send first message (file size)
	String fileSize = intToString(size);
	size_t sizeLength = fileSize.size();
	FileChunk* sizeMessage = new FileChunk(sizeLength + 2);
	char* chunkData = sizeMessage->m_chunk;

	chunkData[0] = kFileStart;
	memcpy(&chunkData[1], fileSize.c_str(), sizeLength);
	chunkData[sizeLength + 1] = '\0';
	events->addEvent(Event(events->forIScreen().fileChunkSending(), eventTarget, sizeMessage));

	// send chunk messages with a fixed chunk size
	size_t sentLength = 0;
	size_t chunkSize = m_chunkSize;
	Stopwatch stopwatch;
	stopwatch.start();
	file.seekg (0, std::ios::beg);
	while (true) {
		if (stopwatch.getTime() > PAUSE_TIME_HACK) {
			// make sure we don't read too much from the mock data.
			if (sentLength + chunkSize > size) {
				chunkSize = size - sentLength;
			}

			// for fileChunk->m_chunk, the first byte is the chunk mark, last is \0
			FileChunk* fileChunk = new FileChunk(chunkSize + 2);
			char* chunkData = fileChunk->m_chunk;

			chunkData[0] = kFileChunk;
			file.read(&chunkData[1], chunkSize);
			chunkData[chunkSize + 1] = '\0';
			events->addEvent(Event(events->forIScreen().fileChunkSending(), eventTarget, fileChunk));

			sentLength += chunkSize;
			file.seekg (sentLength, std::ios::beg);

			if (sentLength == size) {
				break;
			}

			stopwatch.reset();
		}
	}

	// send last message
	FileChunk* transferFinished = new FileChunk(2);
	chunkData = transferFinished->m_chunk;

	chunkData[0] = kFileEnd;
	chunkData[1] = '\0';
	events->addEvent(Event(events->forIScreen().fileChunkSending(), eventTarget, transferFinished));

	file.close();
}

String
FileChunker::intToString(size_t i)
{
	stringstream ss;
	ss << i;
	return ss.str();
}
