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

#include "synergy/ClipboardChunk.h"
#include "synergy/protocol_types.h"

#include "test/global/gtest.h"

TEST (ClipboardChunkTests, start_formatStartChunk) {
<<<<<<< HEAD
    ClipboardID id  = 0;
    UInt32 sequence = 0;
    String mockDataSize ("10");
    ClipboardChunk* chunk = ClipboardChunk::start (id, sequence, mockDataSize);

    EXPECT_EQ (id, chunk->m_chunk[0]);
    EXPECT_EQ (sequence, (UInt32) chunk->m_chunk[1]);
    EXPECT_EQ (kDataStart, chunk->m_chunk[5]);
    EXPECT_EQ ('1', chunk->m_chunk[6]);
    EXPECT_EQ ('0', chunk->m_chunk[7]);
    EXPECT_EQ ('\0', chunk->m_chunk[8]);
=======
    ClipboardID id  = 0;
    UInt32 sequence = 0;
    String mockDataSize ("10");
    ClipboardChunk* chunk = ClipboardChunk::start (id, sequence, mockDataSize);
    UInt32 temp_m_chunk;
    memcpy (&temp_m_chunk, &(chunk->m_chunk[1]), 4);

    EXPECT_EQ (id, chunk->m_chunk[0]);
    EXPECT_EQ (sequence, temp_m_chunk);
    EXPECT_EQ (kDataStart, chunk->m_chunk[5]);
    EXPECT_EQ ('1', chunk->m_chunk[6]);
    EXPECT_EQ ('0', chunk->m_chunk[7]);
    EXPECT_EQ ('\0', chunk->m_chunk[8]);
>>>>>>> master

    delete chunk;
}

TEST (ClipboardChunkTests, data_formatDataChunk) {
<<<<<<< HEAD
    ClipboardID id  = 0;
    UInt32 sequence = 1;
    String mockData ("mock data");
    ClipboardChunk* chunk = ClipboardChunk::data (id, sequence, mockData);

    EXPECT_EQ (id, chunk->m_chunk[0]);
    EXPECT_EQ (sequence, (UInt32) chunk->m_chunk[1]);
    EXPECT_EQ (kDataChunk, chunk->m_chunk[5]);
    EXPECT_EQ ('m', chunk->m_chunk[6]);
    EXPECT_EQ ('o', chunk->m_chunk[7]);
    EXPECT_EQ ('c', chunk->m_chunk[8]);
    EXPECT_EQ ('k', chunk->m_chunk[9]);
    EXPECT_EQ (' ', chunk->m_chunk[10]);
    EXPECT_EQ ('d', chunk->m_chunk[11]);
    EXPECT_EQ ('a', chunk->m_chunk[12]);
    EXPECT_EQ ('t', chunk->m_chunk[13]);
    EXPECT_EQ ('a', chunk->m_chunk[14]);
    EXPECT_EQ ('\0', chunk->m_chunk[15]);
=======
    ClipboardID id  = 0;
    UInt32 sequence = 1;
    String mockData ("mock data");
    ClipboardChunk* chunk = ClipboardChunk::data (id, sequence, mockData);
    UInt32 temp_m_chunk;
    memcpy (&temp_m_chunk, &(chunk->m_chunk[1]), 4);

    EXPECT_EQ (id, chunk->m_chunk[0]);
    EXPECT_EQ (sequence, temp_m_chunk);
    EXPECT_EQ (kDataChunk, chunk->m_chunk[5]);
    EXPECT_EQ ('m', chunk->m_chunk[6]);
    EXPECT_EQ ('o', chunk->m_chunk[7]);
    EXPECT_EQ ('c', chunk->m_chunk[8]);
    EXPECT_EQ ('k', chunk->m_chunk[9]);
    EXPECT_EQ (' ', chunk->m_chunk[10]);
    EXPECT_EQ ('d', chunk->m_chunk[11]);
    EXPECT_EQ ('a', chunk->m_chunk[12]);
    EXPECT_EQ ('t', chunk->m_chunk[13]);
    EXPECT_EQ ('a', chunk->m_chunk[14]);
    EXPECT_EQ ('\0', chunk->m_chunk[15]);
>>>>>>> master

    delete chunk;
}

TEST (ClipboardChunkTests, end_formatDataChunk) {
<<<<<<< HEAD
    ClipboardID id        = 1;
    UInt32 sequence       = 1;
    ClipboardChunk* chunk = ClipboardChunk::end (id, sequence);

    EXPECT_EQ (id, chunk->m_chunk[0]);
    EXPECT_EQ (sequence, (UInt32) chunk->m_chunk[1]);
    EXPECT_EQ (kDataEnd, chunk->m_chunk[5]);
    EXPECT_EQ ('\0', chunk->m_chunk[6]);
=======
    ClipboardID id        = 1;
    UInt32 sequence       = 1;
    ClipboardChunk* chunk = ClipboardChunk::end (id, sequence);
    UInt32 temp_m_chunk;
    memcpy (&temp_m_chunk, &(chunk->m_chunk[1]), 4);

    EXPECT_EQ (id, chunk->m_chunk[0]);
    EXPECT_EQ (sequence, temp_m_chunk);
    EXPECT_EQ (kDataEnd, chunk->m_chunk[5]);
    EXPECT_EQ ('\0', chunk->m_chunk[6]);
>>>>>>> master

    delete chunk;
}
