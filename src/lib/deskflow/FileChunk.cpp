/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileChunk.h"

#include "base/Log.h"
#include "base/Stopwatch.h"
#include "base/String.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/protocol_types.h"
#include "io/IStream.h"

static const uint16_t kIntervalThreshold = 1;

FileChunk::FileChunk(size_t size) : Chunk(size)
{
  m_dataSize = size - FILE_CHUNK_META_SIZE;
}

FileChunk *FileChunk::start(const std::string &size)
{
  size_t sizeLength = size.size();
  FileChunk *start = new FileChunk(sizeLength + FILE_CHUNK_META_SIZE);
  char *chunk = start->m_chunk;
  chunk[0] = kDataStart;
  memcpy(&chunk[1], size.c_str(), sizeLength);
  chunk[sizeLength + 1] = '\0';

  return start;
}

FileChunk *FileChunk::data(uint8_t *data, size_t dataSize)
{
  FileChunk *chunk = new FileChunk(dataSize + FILE_CHUNK_META_SIZE);
  char *chunkData = chunk->m_chunk;
  chunkData[0] = kDataChunk;
  memcpy(&chunkData[1], data, dataSize);
  chunkData[dataSize + 1] = '\0';

  return chunk;
}

FileChunk *FileChunk::end()
{
  FileChunk *end = new FileChunk(FILE_CHUNK_META_SIZE);
  char *chunk = end->m_chunk;
  chunk[0] = kDataEnd;
  chunk[1] = '\0';

  return end;
}

int FileChunk::assemble(deskflow::IStream *stream, std::string &dataReceived, size_t &expectedSize)
{
  // parse
  uint8_t mark = 0;
  std::string content;
  static size_t receivedDataSize;
  static double elapsedTime;
  static Stopwatch stopwatch;

  if (!ProtocolUtil::readf(stream, kMsgDFileTransfer + 4, &mark, &content)) {
    return kError;
  }

  switch (mark) {
  case kDataStart:
    dataReceived.clear();
    expectedSize = deskflow::string::stringToSizeType(content);
    receivedDataSize = 0;
    elapsedTime = 0;
    stopwatch.reset();

    if (CLOG->getFilter() >= kDEBUG2) {
      LOG((CLOG_DEBUG2 "recv file size=%s", content.c_str()));
      stopwatch.start();
    }
    return kStart;

  case kDataChunk:
    dataReceived.append(content);
    if (CLOG->getFilter() >= kDEBUG2) {
      LOG((CLOG_DEBUG2 "recv file chunk size=%i", content.size()));
      double interval = stopwatch.getTime();
      receivedDataSize += content.size();
      LOG((CLOG_DEBUG2 "recv file interval=%f s", interval));
      if (interval >= kIntervalThreshold) {
        double averageSpeed = receivedDataSize / interval / 1000;
        LOG((CLOG_DEBUG2 "recv file average speed=%f kb/s", averageSpeed));

        receivedDataSize = 0;
        elapsedTime += interval;
        stopwatch.reset();
      }
    }
    return kNotFinish;

  case kDataEnd:
    if (expectedSize != dataReceived.size()) {
      LOG((CLOG_ERR "corrupted clipboard data, expected size=%d actual size=%d", expectedSize, dataReceived.size()));
      return kError;
    }

    if (CLOG->getFilter() >= kDEBUG2) {
      LOG((CLOG_DEBUG2 "file transfer finished"));
      elapsedTime += stopwatch.getTime();
      double averageSpeed = expectedSize / elapsedTime / 1000;
      LOG((CLOG_DEBUG2 "file transfer finished: total time consumed=%f s", elapsedTime));
      LOG((CLOG_DEBUG2 "file transfer finished: total data received=%i kb", expectedSize / 1000));
      LOG((CLOG_DEBUG2 "file transfer finished: total average speed=%f kb/s", averageSpeed));
    }
    return kFinish;
  }

  return kError;
}

void FileChunk::send(deskflow::IStream *stream, uint8_t mark, char *data, size_t dataSize)
{
  std::string chunk(data, dataSize);

  switch (mark) {
  case kDataStart:
    LOG((CLOG_DEBUG2 "sending file chunk start: size=%s", data));
    break;

  case kDataChunk:
    LOG((CLOG_DEBUG2 "sending file chunk: size=%i", chunk.size()));
    break;

  case kDataEnd:
    LOG((CLOG_DEBUG2 "sending file finished"));
    break;
  }

  ProtocolUtil::writef(stream, kMsgDFileTransfer, mark, &chunk);
}
