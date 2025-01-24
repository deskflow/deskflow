/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/StreamChunker.h"

#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/Stopwatch.h"
#include "base/String.h"
#include "common/stdexcept.h"
#include "deskflow/ClipboardChunk.h"
#include "deskflow/FileChunk.h"
#include "deskflow/protocol_types.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"

#include <fstream>

using namespace std;

static const size_t g_chunkSize = 512 * 1024; // 512kb

bool StreamChunker::s_isChunkingFile = false;
bool StreamChunker::s_interruptFile = false;
Mutex *StreamChunker::s_interruptMutex = NULL;

void StreamChunker::sendFile(char *filename, IEventQueue *events, void *eventTarget)
{
  s_isChunkingFile = true;

  std::fstream file(static_cast<char *>(filename), std::ios::in | std::ios::binary);

  if (!file.is_open()) {
    throw runtime_error("failed to open file");
  }

  // check file size
  file.seekg(0, std::ios::end);
  size_t size = (size_t)file.tellg();

  // send first message (file size)
  std::string fileSize = deskflow::string::sizeTypeToString(size);
  FileChunk *sizeMessage = FileChunk::start(fileSize);

  events->addEvent(Event(events->forFile().fileChunkSending(), eventTarget, sizeMessage));

  // send chunk messages with a fixed chunk size
  size_t sentLength = 0;
  size_t chunkSize = g_chunkSize;
  file.seekg(0, std::ios::beg);

  while (true) {
    if (s_interruptFile) {
      s_interruptFile = false;
      LOG((CLOG_DEBUG "file transmission interrupted"));
      break;
    }

    events->addEvent(Event(events->forFile().keepAlive(), eventTarget));

    // make sure we don't read too much from the mock data.
    if (sentLength + chunkSize > size) {
      chunkSize = size - sentLength;
    }

    char *chunkData = new char[chunkSize];
    file.read(chunkData, chunkSize);
    uint8_t *data = reinterpret_cast<uint8_t *>(chunkData);
    FileChunk *fileChunk = FileChunk::data(data, chunkSize);
    delete[] chunkData;

    events->addEvent(Event(events->forFile().fileChunkSending(), eventTarget, fileChunk));

    sentLength += chunkSize;
    file.seekg(sentLength, std::ios::beg);

    if (sentLength == size) {
      break;
    }
  }

  // send last message
  FileChunk *end = FileChunk::end();

  events->addEvent(Event(events->forFile().fileChunkSending(), eventTarget, end));

  file.close();

  s_isChunkingFile = false;
}

void StreamChunker::sendClipboard(
    std::string &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events, void *eventTarget
)
{
  // send first message (data size)
  std::string dataSize = deskflow::string::sizeTypeToString(size);
  ClipboardChunk *sizeMessage = ClipboardChunk::start(id, sequence, dataSize);

  events->addEvent(Event(events->forClipboard().clipboardSending(), eventTarget, sizeMessage));

  // send clipboard chunk with a fixed size
  size_t sentLength = 0;
  size_t chunkSize = g_chunkSize;

  while (true) {
    events->addEvent(Event(events->forFile().keepAlive(), eventTarget));

    // make sure we don't read too much from the mock data.
    if (sentLength + chunkSize > size) {
      chunkSize = size - sentLength;
    }

    std::string chunk(data.substr(sentLength, chunkSize).c_str(), chunkSize);
    ClipboardChunk *dataChunk = ClipboardChunk::data(id, sequence, chunk);

    events->addEvent(Event(events->forClipboard().clipboardSending(), eventTarget, dataChunk));

    sentLength += chunkSize;
    if (sentLength == size) {
      break;
    }
  }

  // send last message
  ClipboardChunk *end = ClipboardChunk::end(id, sequence);

  events->addEvent(Event(events->forClipboard().clipboardSending(), eventTarget, end));

  LOG((CLOG_DEBUG "sent clipboard size=%d", sentLength));
}

void StreamChunker::interruptFile()
{
  if (s_isChunkingFile) {
    s_interruptFile = true;
    LOG((CLOG_INFO "previous dragged file has become invalid"));
  }
}
