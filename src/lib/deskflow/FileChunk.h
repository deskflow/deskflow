/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Devs
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Ported from Barrier's FileChunk (barrier/src/lib/barrier/FileChunk.h)
 * to re-enable file drag and drop over kMsgDFileTransfer (DDRG).
 */

#pragma once

#include "deskflow/Chunk.h"
#include "deskflow/ProtocolTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>

constexpr static auto s_fileChunkMetaSize = 2;

namespace deskflow
{
class IStream;
}

struct FileChunkAssemblyState
{
  size_t expectedSize = 0;
  bool active = false;
};

class FileChunk : public Chunk
{
public:
  explicit FileChunk(size_t size);

  static FileChunk *start(const std::string &size);
  static FileChunk *data(const std::string &data);
  static FileChunk *end();

  static TransferState assemble(
      deskflow::IStream *stream, std::string &dataCached, size_t &expectedSize, FileChunkAssemblyState &state
  );

  static void send(deskflow::IStream *stream, void *data);

  static size_t getExpectedSize(const FileChunkAssemblyState &state);
};