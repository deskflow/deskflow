/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016, 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/StreamChunker.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/ClipboardChunk.h"
#include "deskflow/ipc/CoreIpc.h"
#include "io/IStream.h"

#include <algorithm>
#include <memory>

StreamChunker::StreamChunker(IEventQueue *events, deskflow::IStream *stream, const QString &peerName)
    : m_events(events),
      m_stream(stream),
      m_streamTarget(stream->getEventTarget()),
      m_peerName(peerName)
{
  // one chunk per flush, so input written meanwhile isn't queued behind the whole clipboard
  m_events->addHandler(EventTypes::StreamOutputFlushed, m_streamTarget, [this](const auto &) { sendNextChunk(); });
}

StreamChunker::~StreamChunker()
{
  m_events->removeHandler(EventTypes::StreamOutputFlushed, m_streamTarget);
}

void StreamChunker::sendClipboard(std::string data, ClipboardID id, uint32_t sequence)
{
  if (m_sending && id != m_currentId) {
    LOG_DEBUG("queueing clipboard %d behind clipboard %d", id, m_currentId);
    m_queued[id] = Transfer{std::move(data), sequence};
  } else {
    if (m_sending) {
      LOG_DEBUG("restarting clipboard %d transfer with newer data", id);
    }
    beginTransfer(id, Transfer{std::move(data), sequence});
  }
}

void StreamChunker::beginTransfer(ClipboardID id, Transfer transfer)
{
  m_current = std::move(transfer);
  m_currentId = id;
  m_sent = 0;
  m_sending = true;
  ipcSendToClient(
      QStringLiteral("clipboardSending"), QStringLiteral("%1,%2").arg(m_current.data.size()).arg(m_peerName)
  );

  const std::unique_ptr<ClipboardChunk> start(
      ClipboardChunk::start(id, m_current.sequence, std::to_string(m_current.data.size()))
  );
  ClipboardChunk::send(m_stream, *start);
  sendNextChunk();
}

void StreamChunker::sendNextChunk()
{
  if (!m_sending) {
    return;
  }

  if (m_sent < m_current.data.size()) {
    const auto size = std::min(kChunkSize, m_current.data.size() - m_sent);
    const std::unique_ptr<ClipboardChunk> chunk(
        ClipboardChunk::data(m_currentId, m_current.sequence, m_current.data.substr(m_sent, size))
    );
    ClipboardChunk::send(m_stream, *chunk);
    m_sent += size;
  } else {
    const std::unique_ptr<ClipboardChunk> end(ClipboardChunk::end(m_currentId, m_current.sequence));
    ClipboardChunk::send(m_stream, *end);
    LOG_DEBUG("sent clipboard %d, size: %zu", m_currentId, m_current.data.size());
    ipcSendToClient(QStringLiteral("clipboardSent"), m_peerName);

    m_sending = false;
    m_current = {};

    const auto next = std::ranges::find_if(m_queued, [](const auto &queued) { return queued.has_value(); });
    if (next != m_queued.end()) {
      const auto nextId = static_cast<ClipboardID>(next - m_queued.begin());
      auto transfer = std::move(**next);
      next->reset();
      beginTransfer(nextId, std::move(transfer));
    }
  }
}
