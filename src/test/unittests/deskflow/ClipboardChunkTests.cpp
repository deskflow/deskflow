/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClipboardChunk.h"
#include "deskflow/protocol_types.h"

#include <gtest/gtest.h>

TEST(ClipboardChunkTests, start_formatStartChunk)
{
  ClipboardID id = 0;
  uint32_t sequence = 0;
  std::string mockDataSize("10");
  ClipboardChunk *chunk = ClipboardChunk::start(id, sequence, mockDataSize);
  uint32_t temp_m_chunk;
  memcpy(&temp_m_chunk, &(chunk->m_chunk[1]), 4);

  EXPECT_EQ(id, chunk->m_chunk[0]);
  EXPECT_EQ(sequence, temp_m_chunk);
  EXPECT_EQ(kDataStart, chunk->m_chunk[5]);
  EXPECT_EQ('1', chunk->m_chunk[6]);
  EXPECT_EQ('0', chunk->m_chunk[7]);
  EXPECT_EQ('\0', chunk->m_chunk[8]);

  delete chunk;
}

TEST(ClipboardChunkTests, data_formatDataChunk)
{
  ClipboardID id = 0;
  uint32_t sequence = 1;
  std::string mockData("mock data");
  ClipboardChunk *chunk = ClipboardChunk::data(id, sequence, mockData);

  EXPECT_EQ(id, chunk->m_chunk[0]);
  EXPECT_EQ(sequence, (uint32_t)chunk->m_chunk[1]);
  EXPECT_EQ(kDataChunk, chunk->m_chunk[5]);
  EXPECT_EQ('m', chunk->m_chunk[6]);
  EXPECT_EQ('o', chunk->m_chunk[7]);
  EXPECT_EQ('c', chunk->m_chunk[8]);
  EXPECT_EQ('k', chunk->m_chunk[9]);
  EXPECT_EQ(' ', chunk->m_chunk[10]);
  EXPECT_EQ('d', chunk->m_chunk[11]);
  EXPECT_EQ('a', chunk->m_chunk[12]);
  EXPECT_EQ('t', chunk->m_chunk[13]);
  EXPECT_EQ('a', chunk->m_chunk[14]);
  EXPECT_EQ('\0', chunk->m_chunk[15]);

  delete chunk;
}

TEST(ClipboardChunkTests, end_formatDataChunk)
{
  ClipboardID id = 1;
  uint32_t sequence = 1;
  ClipboardChunk *chunk = ClipboardChunk::end(id, sequence);

  EXPECT_EQ(id, chunk->m_chunk[0]);
  EXPECT_EQ(sequence, (uint32_t)chunk->m_chunk[1]);
  EXPECT_EQ(kDataEnd, chunk->m_chunk[5]);
  EXPECT_EQ('\0', chunk->m_chunk[6]);

  delete chunk;
}
