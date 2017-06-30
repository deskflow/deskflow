/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#pragma once

#include "synergy/Chunk.h"
#include "base/String.h"
#include "common/basic_types.h"

#define FILE_CHUNK_META_SIZE 2

namespace synergy {
class IStream;
};

class FileChunk : public Chunk {
public:
    FileChunk (size_t size);

    static FileChunk* start (const String& size);
    static FileChunk* data (UInt8* data, size_t dataSize);
    static FileChunk* end ();
    static int assemble (synergy::IStream* stream, String& dataCached,
                         size_t& expectedSize);
    static void
    send (synergy::IStream* stream, UInt8 mark, char* data, size_t dataSize);
};
