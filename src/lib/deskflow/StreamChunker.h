/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016, 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <QString>

class IEventQueue;

namespace deskflow {
class IStream;
}

class StreamChunker
{
public:
  StreamChunker(IEventQueue *events, deskflow::IStream *stream, const QString &peerName);
  StreamChunker(StreamChunker const &) = delete;
  StreamChunker(StreamChunker &&) = delete;
  ~StreamChunker();

  StreamChunker &operator=(StreamChunker const &) = delete;
  StreamChunker &operator=(StreamChunker &&) = delete;

  void sendClipboard(std::string data, ClipboardID id, uint32_t sequence);

private:
  struct Transfer
  {
    std::string data;
    uint32_t sequence = 0;
  };

  void beginTransfer(ClipboardID id, Transfer transfer);
  void sendNextChunk();

  static constexpr size_t kChunkSize = 64 * 1024;

  IEventQueue *m_events;
  deskflow::IStream *m_stream;
  void *m_streamTarget;
  QString m_peerName;
  Transfer m_current;
  ClipboardID m_currentId = 0;
  size_t m_sent = 0;
  bool m_sending = false;
  std::array<std::optional<Transfer>, kClipboardEnd> m_queued;
};
