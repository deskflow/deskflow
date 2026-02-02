/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClipboardChunk.h"

#include "base/Log.h"
#include "base/String.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"

#include <cstddef>
#include <cstring>

size_t ClipboardChunk::s_expectedSize = 0;

ClipboardChunk::ClipboardChunk(size_t size) : Chunk(size)
{
  m_dataSize = size - s_clipboardChunkMetaSize;
}

ClipboardChunk *ClipboardChunk::start(ClipboardID id, uint32_t sequence, const std::string &size)
{
  size_t sizeLength = size.size();
  auto *start = new ClipboardChunk(sizeLength + s_clipboardChunkMetaSize);
  char *chunk = start->m_chunk;

  chunk[0] = id;
  std::memcpy(&chunk[1], &sequence, 4);
  chunk[5] = ChunkType::DataStart;
  memcpy(&chunk[6], size.c_str(), sizeLength);
  chunk[sizeLength + s_clipboardChunkMetaSize - 1] = '\0';

  return start;
}

ClipboardChunk *ClipboardChunk::data(ClipboardID id, uint32_t sequence, const std::string &data)
{
  size_t dataSize = data.size();
  auto *chunk = new ClipboardChunk(dataSize + s_clipboardChunkMetaSize);
  char *chunkData = chunk->m_chunk;

  chunkData[0] = id;
  std::memcpy(&chunkData[1], &sequence, 4);
  chunkData[5] = ChunkType::DataChunk;
  memcpy(&chunkData[6], data.c_str(), dataSize);
  chunkData[dataSize + s_clipboardChunkMetaSize - 1] = '\0';

  return chunk;
}

ClipboardChunk *ClipboardChunk::end(ClipboardID id, uint32_t sequence)
{
  auto *end = new ClipboardChunk(s_clipboardChunkMetaSize);
  char *chunk = end->m_chunk;

  chunk[0] = id;
  std::memcpy(&chunk[1], &sequence, 4);
  chunk[5] = ChunkType::DataEnd;
  chunk[s_clipboardChunkMetaSize - 1] = '\0';
  return end;
}

TransferState
ClipboardChunk::assemble(deskflow::IStream *stream, std::string &dataCached, ClipboardID &id, uint32_t &sequence)
{
  using enum TransferState;
  uint8_t mark;
  std::string data;

  if (!ProtocolUtil::readf(stream, kMsgDClipboard + 4, &id, &sequence, &mark, &data)) {
    LOG_ERR("clipboard: failed to read clipboard chunk from stream (protocol error or connection lost)");
    return Error;
  }

  if (mark == ChunkType::DataStart) {
    bool ok = false;
    s_expectedSize = QString::fromStdString(data).toULong(&ok);
    if (!ok || data.empty()) {
      LOG_ERR("clipboard: invalid size in DataStart chunk, data='%s'", data.c_str());
      return Error;
    }
    LOG_DEBUG("clipboard: start receiving clipboard %d, expected size=%zu", id, s_expectedSize);
    dataCached.clear();
    return Started;
  } else if (mark == ChunkType::DataChunk) {
    dataCached.append(data);
    LOG_DEBUG2("clipboard: received data chunk, accumulated %zu of %zu bytes", dataCached.size(), s_expectedSize);
    return TransferState::InProgress;
  } else if (mark == ChunkType::DataEnd) {
    // validate
    if (id >= kClipboardEnd) {
      LOG_ERR("clipboard: invalid clipboard id %d (max=%d)", id, kClipboardEnd - 1);
      return Error;
    } else if (s_expectedSize != dataCached.size()) {
      LOG_ERR(
          "clipboard: corrupted data, expected size=%zu actual size=%zu (lost %zd bytes)", s_expectedSize,
          dataCached.size(), static_cast<ssize_t>(s_expectedSize) - static_cast<ssize_t>(dataCached.size())
      );
      return Error;
    }
    LOG_DEBUG("clipboard: finished receiving clipboard %d, size=%zu", id, dataCached.size());
    return Finished;
  }

  LOG_ERR("clipboard: unknown chunk mark type %d", mark);
  return Error;
}

void ClipboardChunk::send(deskflow::IStream *stream, void *data)
{
  const auto *clipboardData = static_cast<ClipboardChunk *>(data);

  LOG_DEBUG1("sending clipboard chunk");

  const char *chunk = clipboardData->m_chunk;
  ClipboardID id = chunk[0];
  uint32_t sequence;
  std::memcpy(&sequence, &chunk[1], 4);
  uint8_t mark = chunk[5];
  std::string dataChunk(&chunk[6], clipboardData->m_dataSize);

  switch (mark) {
  case ChunkType::DataStart:
    LOG_DEBUG2("sending clipboard chunk start: size=%s", dataChunk.c_str());
    break;

  case ChunkType::DataChunk:
    LOG_DEBUG2("sending clipboard chunk data: size=%i", dataChunk.size());
    break;

  case ChunkType::DataEnd:
    LOG_DEBUG2("sending clipboard finished");
    break;

  default:
    break;
  }

  ProtocolUtil::writef(stream, kMsgDClipboard, id, sequence, mark, &dataChunk);
}
