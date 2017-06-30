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
#include "synergy/clipboard_types.h"
#include "base/String.h"
#include "common/basic_types.h"

#define CLIPBOARD_CHUNK_META_SIZE 7

namespace synergy {
class IStream;
};

class ClipboardChunk : public Chunk {
public:
    ClipboardChunk (size_t size);

    static ClipboardChunk*
    start (ClipboardID id, UInt32 sequence, const String& size);
    static ClipboardChunk*
    data (ClipboardID id, UInt32 sequence, const String& data);
    static ClipboardChunk* end (ClipboardID id, UInt32 sequence);

    static int assemble (synergy::IStream* stream, String& dataCached,
                         ClipboardID& id, UInt32& sequence);

    static void send (synergy::IStream* stream, void* data);

    static size_t
    getExpectedSize () {
        return s_expectedSize;
    }

private:
    static size_t s_expectedSize;
};
