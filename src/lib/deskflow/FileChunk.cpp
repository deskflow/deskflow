/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Devs
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Ported from Barrier's FileChunk (barrier/src/lib/barrier/FileChunk.cpp)
 * to re-enable file drag and drop over kMsgDFileTransfer (DDRG).
 */

#include "deskflow/FileChunk.h"

#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"

#include <QString>
#include <cstring>
#include <limits>

namespace {

void clearCachedData(std::string &dataCached)
{
  dataCached.clear();
  dataCached.shrink_to_fit();
}

} // namespace

FileChunk::FileChunk(size_t size) : Chunk(size)
{
  m_dataSize = size - s_fileChunkMetaSize;
}

FileChunk *FileChunk::start(const std::string &size)
{
  size_t sizeLength = size.size();
  auto *start = new FileChunk(sizeLength + s_fileChunkMetaSize);
  char *chunk = start->m_chunk;

  chunk[0] = ChunkType::DataStart;
  std::memcpy(&chunk[1], size.c_str(), sizeLength);
  chunk[sizeLength + 1] = '\0';

  return start;
}

FileChunk *FileChunk::data(const std::string &data)
{
  size_t dataSize = data.size();
  auto *chunk = new FileChunk(dataSize + s_fileChunkMetaSize);
  char *chunkData = chunk->m_chunk;

  chunkData[0] = ChunkType::DataChunk;
  std::memcpy(&chunkData[1], data.c_str(), dataSize);
  chunkData[dataSize + 1] = '\0';

  return chunk;
}

FileChunk *FileChunk::end()
{
  auto *end = new FileChunk(s_fileChunkMetaSize);
  char *chunk = end->m_chunk;

  chunk[0] = ChunkType::DataEnd;
  chunk[1] = '\0';

  return end;
}

TransferState FileChunk::assemble(
    deskflow::IStream *stream, std::string &dataCached, size_t &expectedSize, FileChunkAssemblyState &state
)
{
  using enum TransferState;
  uint8_t mark;
  std::string data;
  auto reset = [&]() {
    state = {};
    clearCachedData(dataCached);
  };

  if (!ProtocolUtil::readf(stream, kMsgDFileTransfer + 4, &mark, &data)) {
    reset();
    return Error;
  }

  if (mark == ChunkType::DataStart) {
    bool ok = false;
    const auto expected = QString::fromStdString(data).toULongLong(&ok);
    if (!ok || expected > std::numeric_limits<size_t>::max()) {
      LOG_ERR("file transfer invalid size header: %s", data.c_str());
      reset();
      return Error;
    }

    clearCachedData(dataCached);
    expectedSize = static_cast<size_t>(expected);
    state.expectedSize = expectedSize;
    state.active = true;

    LOG_DEBUG("start receiving file data, expected size=%zu", state.expectedSize);
    return Started;
  } else if (mark == ChunkType::DataChunk) {
    if (!state.active) {
      LOG_ERR("file data chunk before start");
      reset();
      return Error;
    }

    dataCached.append(data);
    if (dataCached.size() > state.expectedSize) {
      LOG_ERR("file data exceeds declared size, expected=%zu got=%zu", state.expectedSize, dataCached.size());
      reset();
      return Error;
    }

    return InProgress;
  } else if (mark == ChunkType::DataEnd) {
    if (!state.active) {
      LOG_ERR("file data end before start");
      reset();
      return Error;
    }
    if (dataCached.size() != state.expectedSize) {
      LOG_ERR("corrupted file data, expected size=%zu actual size=%zu", state.expectedSize, dataCached.size());
      reset();
      return Error;
    }

    LOG_DEBUG("file transfer finished, size=%zu", dataCached.size());
    state = {};
    return Finished;
  }

  LOG_ERR("file transfer unknown mark: %d", mark);
  reset();
  return Error;
}

void FileChunk::send(deskflow::IStream *stream, void *data)
{
  const auto *fileData = static_cast<FileChunk *>(data);

  LOG_VERBOSE("sending file chunk");

  const char *chunk = fileData->m_chunk;
  uint8_t mark = chunk[0];
  std::string dataChunk(&chunk[1], fileData->m_dataSize);

  switch (mark) {
  case ChunkType::DataStart:
    LOG_DEBUG("sending file chunk start: size=%s", dataChunk.c_str());
    break;
  case ChunkType::DataChunk:
    LOG_DEBUG("sending file chunk data: size=%zu", dataChunk.size());
    break;
  case ChunkType::DataEnd:
    LOG_DEBUG("sending file finished");
    break;
  default:
    LOG_ERR("file chunk unknown mark: %d", mark);
    return;
  }

  ProtocolUtil::writef(stream, kMsgDFileTransfer, mark, &dataChunk);
}

size_t FileChunk::getExpectedSize(const FileChunkAssemblyState &state)
{
  return state.expectedSize;
}