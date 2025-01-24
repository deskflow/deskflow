/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"
#include "deskflow/Chunk.h"

#include <string>

#define FILE_CHUNK_META_SIZE 2

namespace deskflow {
class IStream;
};

class FileChunk : public Chunk
{
public:
  FileChunk(size_t size);

  static FileChunk *start(const std::string &size);
  static FileChunk *data(uint8_t *data, size_t dataSize);
  static FileChunk *end();
  static int assemble(deskflow::IStream *stream, std::string &dataCached, size_t &expectedSize);
  static void send(deskflow::IStream *stream, uint8_t mark, char *data, size_t dataSize);
};
