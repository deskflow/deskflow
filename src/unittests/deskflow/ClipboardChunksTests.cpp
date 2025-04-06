/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardChunksTests.h"

#include "../../lib/deskflow/ClipboardChunk.h"

#include "deskflow/ProtocolTypes.h"

void ClipboardChunksTests::startFormatData()
{
  ClipboardID id = 0;
  uint32_t sequence = 0;
  std::string mockDataSize("10");
  ClipboardChunk *chunk = ClipboardChunk::start(id, sequence, mockDataSize);
  uint32_t temp_m_chunk;
  memcpy(&temp_m_chunk, &(chunk->m_chunk[1]), 4);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE(temp_m_chunk, sequence);
  QCOMPARE(chunk->m_chunk[5], kDataStart);
  QCOMPARE(chunk->m_chunk[6], '1');
  QCOMPARE(chunk->m_chunk[7], '0');
  QCOMPARE(chunk->m_chunk[8], '\0');
  delete chunk;
}

void ClipboardChunksTests::formatDataChunk()
{
  ClipboardID id = 0;
  uint32_t sequence = 1;
  std::string mockData("mock data");
  ClipboardChunk *chunk = ClipboardChunk::data(id, sequence, mockData);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE((uint32_t)chunk->m_chunk[1], sequence);
  QCOMPARE(chunk->m_chunk[5], kDataChunk);
  QCOMPARE(chunk->m_chunk[6], 'm');
  QCOMPARE(chunk->m_chunk[7], 'o');
  QCOMPARE(chunk->m_chunk[8], 'c');
  QCOMPARE(chunk->m_chunk[9], 'k');
  QCOMPARE(chunk->m_chunk[10], ' ');
  QCOMPARE(chunk->m_chunk[11], 'd');
  QCOMPARE(chunk->m_chunk[12], 'a');
  QCOMPARE(chunk->m_chunk[13], 't');
  QCOMPARE(chunk->m_chunk[14], 'a');
  QCOMPARE(chunk->m_chunk[15], '\0');

  delete chunk;
}

void ClipboardChunksTests::endFormatData()
{
  ClipboardID id = 1;
  uint32_t sequence = 1;
  ClipboardChunk *chunk = ClipboardChunk::end(id, sequence);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE((uint32_t)chunk->m_chunk[1], sequence);
  QCOMPARE(chunk->m_chunk[5], kDataEnd);
  QCOMPARE(chunk->m_chunk[6], '\0');

  delete chunk;
}

QTEST_MAIN(ClipboardChunksTests)
