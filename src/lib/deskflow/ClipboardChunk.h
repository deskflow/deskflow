/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"
#include "deskflow/Chunk.h"
#include "deskflow/clipboard_types.h"

#include <string>

#define CLIPBOARD_CHUNK_META_SIZE 7

namespace deskflow {
class IStream;
};

class ClipboardChunk : public Chunk
{
public:
  ClipboardChunk(size_t size);

  static ClipboardChunk *start(ClipboardID id, uint32_t sequence, const std::string &size);
  static ClipboardChunk *data(ClipboardID id, uint32_t sequence, const std::string &data);
  static ClipboardChunk *end(ClipboardID id, uint32_t sequence);

  static int assemble(deskflow::IStream *stream, std::string &dataCached, ClipboardID &id, uint32_t &sequence);

  static void send(deskflow::IStream *stream, void *data);

  static size_t getExpectedSize()
  {
    return s_expectedSize;
  }

private:
  static size_t s_expectedSize;
};
