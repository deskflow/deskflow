/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/StreamChunker.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/ClipboardChunk.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"

#include <algorithm>
#include <memory>
#include <string>

namespace {

static const size_t g_chunkSize = 64 * 1024; // 64kb

class ClipboardSendChunk : public EventData
{
public:
  enum class Type
  {
    Start,
    Data,
    End
  };

  ClipboardSendChunk(
      std::shared_ptr<const std::string> data, size_t size, size_t offset, ClipboardID id, uint32_t sequence, Type type
  )
      : m_data(std::move(data)),
        m_size(size),
        m_offset(offset),
        m_id(id),
        m_sequence(sequence),
        m_type(type)
  {
    // do nothing
  }

  std::shared_ptr<const std::string> m_data;
  size_t m_size;
  size_t m_offset;
  ClipboardID m_id;
  uint32_t m_sequence;
  Type m_type;
};

void queueClipboardSendChunk(
    IEventQueue *events, void *eventTarget, std::shared_ptr<const std::string> data, size_t size, size_t offset,
    ClipboardID id, uint32_t sequence, ClipboardSendChunk::Type type
)
{
  events->addEvent(
      Event(EventTypes::ClipboardSending, eventTarget, new ClipboardSendChunk(data, size, offset, id, sequence, type))
  );
}

} // namespace

void StreamChunker::sendClipboard(
    const std::string_view &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events, void *eventTarget
)
{
  const size_t payloadSize = std::min(size, data.size());
  auto payload = std::make_shared<const std::string>(data.substr(0, payloadSize));

  queueClipboardSendChunk(events, eventTarget, payload, payloadSize, 0, id, sequence, ClipboardSendChunk::Type::Start);

  LOG_DEBUG("queued clipboard size=%d", payloadSize);
}

void StreamChunker::sendClipboardChunk(
    deskflow::IStream *stream, EventData *data, IEventQueue *events, void *eventTarget
)
{
  auto *sendChunk = dynamic_cast<ClipboardSendChunk *>(data);
  if (sendChunk == nullptr) {
    ClipboardChunk::send(stream, data);
    return;
  }

  uint8_t mark = 0;
  std::string chunkData;
  bool queueNext = false;
  size_t nextOffset = sendChunk->m_offset;
  ClipboardSendChunk::Type nextType = ClipboardSendChunk::Type::End;

  switch (sendChunk->m_type) {
  case ClipboardSendChunk::Type::Start:
    mark = ChunkType::DataStart;
    chunkData = QString::number(sendChunk->m_size).toStdString();
    queueNext = true;
    nextType = (sendChunk->m_size == 0) ? ClipboardSendChunk::Type::End : ClipboardSendChunk::Type::Data;
    break;

  case ClipboardSendChunk::Type::Data: {
    mark = ChunkType::DataChunk;
    const size_t remaining = sendChunk->m_size - sendChunk->m_offset;
    const size_t chunkSize = std::min(g_chunkSize, remaining);
    chunkData.assign(sendChunk->m_data->data() + sendChunk->m_offset, chunkSize);
    nextOffset = sendChunk->m_offset + chunkSize;
    queueNext = true;
    nextType = (nextOffset < sendChunk->m_size) ? ClipboardSendChunk::Type::Data : ClipboardSendChunk::Type::End;
    break;
  }

  case ClipboardSendChunk::Type::End:
    mark = ChunkType::DataEnd;
    LOG_DEBUG("sent clipboard size=%d", sendChunk->m_size);
    break;
  }

  ProtocolUtil::writef(stream, kMsgDClipboard, sendChunk->m_id, sendChunk->m_sequence, mark, &chunkData);

  if (queueNext) {
    queueClipboardSendChunk(
        events, eventTarget, sendChunk->m_data, sendChunk->m_size, nextOffset, sendChunk->m_id, sendChunk->m_sequence,
        nextType
    );
  }
}
