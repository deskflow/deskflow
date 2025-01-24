/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClipboardChunk.h"

#include "base/Log.h"
#include "base/String.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/protocol_types.h"
#include "io/IStream.h"
#include <cstring>

size_t ClipboardChunk::s_expectedSize = 0;

ClipboardChunk::ClipboardChunk(size_t size) : Chunk(size)
{
  m_dataSize = size - CLIPBOARD_CHUNK_META_SIZE;
}

ClipboardChunk *ClipboardChunk::start(ClipboardID id, uint32_t sequence, const std::string &size)
{
  size_t sizeLength = size.size();
  ClipboardChunk *start = new ClipboardChunk(sizeLength + CLIPBOARD_CHUNK_META_SIZE);
  char *chunk = start->m_chunk;

  chunk[0] = id;
  std::memcpy(&chunk[1], &sequence, 4);
  chunk[5] = kDataStart;
  memcpy(&chunk[6], size.c_str(), sizeLength);
  chunk[sizeLength + CLIPBOARD_CHUNK_META_SIZE - 1] = '\0';

  return start;
}

ClipboardChunk *ClipboardChunk::data(ClipboardID id, uint32_t sequence, const std::string &data)
{
  size_t dataSize = data.size();
  ClipboardChunk *chunk = new ClipboardChunk(dataSize + CLIPBOARD_CHUNK_META_SIZE);
  char *chunkData = chunk->m_chunk;

  chunkData[0] = id;
  std::memcpy(&chunkData[1], &sequence, 4);
  chunkData[5] = kDataChunk;
  memcpy(&chunkData[6], data.c_str(), dataSize);
  chunkData[dataSize + CLIPBOARD_CHUNK_META_SIZE - 1] = '\0';

  return chunk;
}

ClipboardChunk *ClipboardChunk::end(ClipboardID id, uint32_t sequence)
{
  ClipboardChunk *end = new ClipboardChunk(CLIPBOARD_CHUNK_META_SIZE);
  char *chunk = end->m_chunk;

  chunk[0] = id;
  std::memcpy(&chunk[1], &sequence, 4);
  chunk[5] = kDataEnd;
  chunk[CLIPBOARD_CHUNK_META_SIZE - 1] = '\0';

  return end;
}

int ClipboardChunk::assemble(deskflow::IStream *stream, std::string &dataCached, ClipboardID &id, uint32_t &sequence)
{
  uint8_t mark;
  std::string data;

  if (!ProtocolUtil::readf(stream, kMsgDClipboard + 4, &id, &sequence, &mark, &data)) {
    return kError;
  }

  if (mark == kDataStart) {
    s_expectedSize = deskflow::string::stringToSizeType(data);
    LOG((CLOG_DEBUG "start receiving clipboard data"));
    dataCached.clear();
    return kStart;
  } else if (mark == kDataChunk) {
    dataCached.append(data);
    return kNotFinish;
  } else if (mark == kDataEnd) {
    // validate
    if (id >= kClipboardEnd) {
      return kError;
    } else if (s_expectedSize != dataCached.size()) {
      LOG((CLOG_ERR "corrupted clipboard data, expected size=%d actual size=%d", s_expectedSize, dataCached.size()));
      return kError;
    }
    return kFinish;
  }

  LOG((CLOG_ERR "clipboard transmission failed: unknown error"));
  return kError;
}

void ClipboardChunk::send(deskflow::IStream *stream, void *data)
{
  ClipboardChunk *clipboardData = static_cast<ClipboardChunk *>(data);

  LOG((CLOG_DEBUG1 "sending clipboard chunk"));

  char *chunk = clipboardData->m_chunk;
  ClipboardID id = chunk[0];
  uint32_t sequence;
  std::memcpy(&sequence, &chunk[1], 4);
  uint8_t mark = chunk[5];
  std::string dataChunk(&chunk[6], clipboardData->m_dataSize);

  switch (mark) {
  case kDataStart:
    LOG((CLOG_DEBUG2 "sending clipboard chunk start: size=%s", dataChunk.c_str()));
    break;

  case kDataChunk:
    LOG((CLOG_DEBUG2 "sending clipboard chunk data: size=%i", dataChunk.size()));
    break;

  case kDataEnd:
    LOG((CLOG_DEBUG2 "sending clipboard finished"));
    break;
  }

  ProtocolUtil::writef(stream, kMsgDClipboard, id, sequence, mark, &dataChunk);
}
