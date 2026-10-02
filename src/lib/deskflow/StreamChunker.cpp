/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/StreamChunker.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/ClipboardChunk.h"
#include "deskflow/DragInformation.h"
#include "deskflow/FileChunk.h"
#include "deskflow/ProtocolTypes.h"

#include <QString>
#include <fstream>
#include <stdexcept>

static const size_t g_chunkSize = 512 * 1024; // 512kb

void StreamChunker::sendClipboard(
    const std::string_view &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events, void *eventTarget
)
{
  // send first message (data size)
  std::string dataSize = QString::number(size).toStdString();
  ClipboardChunk *sizeMessage = ClipboardChunk::start(id, sequence, dataSize);

  events->addEvent(Event(EventTypes::ClipboardSending, eventTarget, sizeMessage));

  // send clipboard chunk with a fixed size
  size_t sentLength = 0;
  size_t chunkSize = g_chunkSize;

  while (true) {
    // make sure we don't read too much from the mock data.
    if (sentLength + chunkSize > size) {
      chunkSize = size - sentLength;
    }

    std::string chunk(data.substr(sentLength, chunkSize).data(), chunkSize);
    ClipboardChunk *dataChunk = ClipboardChunk::data(id, sequence, chunk);

    events->addEvent(Event(EventTypes::ClipboardSending, eventTarget, dataChunk));

    sentLength += chunkSize;
    if (sentLength == size) {
      break;
    }
  }

  // send last message
  ClipboardChunk *end = ClipboardChunk::end(id, sequence);

  events->addEvent(Event(EventTypes::ClipboardSending, eventTarget, end));

  LOG_DEBUG("sent clipboard size=%d", sentLength);
}

void StreamChunker::sendFile(const std::string &filename, IEventQueue *events, void *eventTarget)
{
  std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file");
  }

  // check file size
  file.seekg(0, std::ios::end);
  size_t size = static_cast<size_t>(file.tellg());

  // send first message (file size)
  std::string fileSize = QString::number(size).toStdString();
  FileChunk *sizeMessage = FileChunk::start(fileSize);
  events->addEvent(Event(EventTypes::FileChunkSending, eventTarget, sizeMessage));

  // send chunk messages with a fixed chunk size
  size_t sentLength = 0;
  size_t chunkSize = g_chunkSize;
  file.seekg(0, std::ios::beg);

  while (true) {
    if (sentLength + chunkSize > size) {
      chunkSize = size - sentLength;
    }

    std::string chunk(chunkSize, '\0');
    file.read(chunk.data(), static_cast<std::streamsize>(chunkSize));
    FileChunk *fileChunk = FileChunk::data(chunk);
    events->addEvent(Event(EventTypes::FileChunkSending, eventTarget, fileChunk));

    sentLength += chunkSize;
    if (sentLength == size) {
      break;
    }
  }

  // send last message
  FileChunk *end = FileChunk::end();
  events->addEvent(Event(EventTypes::FileChunkSending, eventTarget, end));

  file.close();
  LOG_DEBUG("sent file size=%zu", sentLength);
}

void StreamChunker::sendDragInfo(const std::string &info, uint32_t fileCount, IEventQueue *events, void *eventTarget)
{
  auto *data = new DragInfoEventData;
  data->fileCount = fileCount;
  data->info = info;
  events->addEvent(Event(EventTypes::DragInfoSending, eventTarget, data));
}
