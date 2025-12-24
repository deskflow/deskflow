/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardChunksTests.h"

#include "deskflow/ClipboardChunk.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>

namespace {

// Minimal read-only in-memory stream for feeding ProtocolUtil::readf() data.
class MemoryStream : public deskflow::IStream
{
public:
  void push(const std::string &bytes)
  {
    m_q.push_back(bytes);
  }

  uint32_t read(void *buffer, uint32_t n) override
  {
    if (m_inputShutdown || m_q.empty() || n == 0) {
      return 0;
    }
    auto &front = m_q.front();
    const size_t take = std::min(static_cast<size_t>(n), front.size());
    if (buffer != nullptr) {
      std::memcpy(buffer, front.data(), take);
    }
    front.erase(0, take);
    if (front.empty()) {
      m_q.pop_front();
    }
    return static_cast<uint32_t>(take);
  }

  void write(const void *, uint32_t) override
  {
  }

  void close() override
  {
    m_q.clear();
    m_inputShutdown = true;
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
    m_q.clear();
    m_inputShutdown = true;
  }

  void shutdownOutput() override
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<MemoryStream *>(this);
  }

  bool isReady() const override
  {
    return !m_inputShutdown && !m_q.empty();
  }

  uint32_t getSize() const override
  {
    if (m_inputShutdown || m_q.empty()) {
      return 0;
    }

    size_t total = 0;
    for (const auto &chunk : m_q) {
      total += chunk.size();
      if (total >= 0xFFFFFFFFu) {
        return 0xFFFFFFFFu;
      }
    }
    return static_cast<uint32_t>(total);
  }

private:
  std::deque<std::string> m_q;
  bool m_inputShutdown = false;
};

// Minimal write-only stream so we can use ProtocolUtil::writef() to encode messages.
class BufferWriteStream : public deskflow::IStream
{
public:
  const std::string &str() const
  {
    return m_buf;
  }

  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }

  void write(const void *buffer, uint32_t n) override
  {
    if (m_outputShutdown || n == 0) {
      return;
    }
    m_buf.append(static_cast<const char *>(buffer), static_cast<size_t>(n));
  }

  void close() override
  {
    m_outputShutdown = true;
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
  }

  void shutdownOutput() override
  {
    m_outputShutdown = true;
  }

  void *getEventTarget() const override
  {
    return const_cast<BufferWriteStream *>(this);
  }

  bool isReady() const override
  {
    return false;
  }

  uint32_t getSize() const override
  {
    return 0;
  }

private:
  std::string m_buf;
  bool m_outputShutdown = false;
};

static std::string encodeClipboardMsg(ClipboardID id, uint32_t seq, uint8_t mark, const std::string &data)
{
  BufferWriteStream ws;
  std::string payload = data;
  // ClipboardChunk::assemble() reads using `kMsgDClipboard + 4` (without the "DCLP" prefix).
  ProtocolUtil::writef(&ws, kMsgDClipboard + 4, id, seq, mark, &payload);
  return ws.str();
}

} // namespace

void ClipboardChunksTests::initTestCase()
{
  // ProtocolUtil emits LOG_* messages; unit tests must create the Log singleton.
  m_log.setFilter(LogLevel::Debug);
}

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
  QCOMPARE(chunk->m_chunk[5], ChunkType::DataStart);
  QCOMPARE(chunk->m_chunk[6], '1');
  QCOMPARE(chunk->m_chunk[7], '0');
  QCOMPARE(chunk->m_chunk[8], '\0');
  delete chunk;
}

void ClipboardChunksTests::formatDataChunk()
{
  ClipboardID id = 0;
  uint32_t sequence = 1;
  uint32_t temp_m_chunk;
  std::string mockData("mock data");
  ClipboardChunk *chunk = ClipboardChunk::data(id, sequence, mockData);
  memcpy(&temp_m_chunk, &chunk->m_chunk[1], 4);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE(temp_m_chunk, sequence);
  QCOMPARE(chunk->m_chunk[5], ChunkType::DataChunk);
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
  uint32_t temp_m_chunk;
  ClipboardChunk *chunk = ClipboardChunk::end(id, sequence);
  memcpy(&temp_m_chunk, &chunk->m_chunk[1], 4);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE(temp_m_chunk, sequence);
  QCOMPARE(chunk->m_chunk[5], ChunkType::DataEnd);
  QCOMPARE(chunk->m_chunk[6], '\0');

  delete chunk;
}

// Regression test: expected size must be parsed as a number, not digit-count.
void ClipboardChunksTests::assembleParsesExpectedSizeAsNumber()
{
  MemoryStream ms;

  constexpr ClipboardID id = 0;
  constexpr uint32_t seq = 1234;

  // "1000000" has 7 digits; old bug treated expected size as 7 bytes.
  ms.push(encodeClipboardMsg(id, seq, ChunkType::DataStart, "1000000"));

  // Keep payload modest for unit tests, but still large enough to validate the behavior.
  // Total = 1,000,000 bytes; expected size is large to ensure we parse the header as a number.
  ms.push(encodeClipboardMsg(id, seq, ChunkType::DataChunk, std::string(500'000, 'A')));
  ms.push(encodeClipboardMsg(id, seq, ChunkType::DataChunk, std::string(500'000, 'B')));
  ms.push(encodeClipboardMsg(id, seq, ChunkType::DataEnd, ""));

  std::string cached;
  ClipboardID outId = kClipboardEnd;
  uint32_t outSeq = 0;

  QCOMPARE(ClipboardChunk::assemble(&ms, cached, outId, outSeq), TransferState::Started);
  QCOMPARE(ClipboardChunk::assemble(&ms, cached, outId, outSeq), TransferState::InProgress);
  QCOMPARE(ClipboardChunk::assemble(&ms, cached, outId, outSeq), TransferState::InProgress);
  QCOMPARE(ClipboardChunk::assemble(&ms, cached, outId, outSeq), TransferState::Finished);

  QCOMPARE(outId, id);
  QCOMPARE(outSeq, seq);
  // Must be the numeric value (1,000,000), not the digit count (7).
  QCOMPARE(ClipboardChunk::getExpectedSize(), static_cast<size_t>(1'000'000));
  QCOMPARE(cached.size(), static_cast<size_t>(1'000'000));
  QCOMPARE(cached[0], 'A');
  QCOMPARE(cached[500'000], 'B');
}

QTEST_MAIN(ClipboardChunksTests)
