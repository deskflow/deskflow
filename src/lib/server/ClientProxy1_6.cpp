/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_6.h"

#include "base/Log.h"
#include "deskflow/ClipboardChunk.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/StreamChunker.h"
#include "io/IStream.h"
#include "server/Server.h"

//
// ClientProxy1_6
//

ClientProxy1_6::ClientProxy1_6(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_5(name, stream, server, events),
      m_events(events)
{
  m_events->addHandler(EventTypes::ClipboardSending, this, [this](const auto &e) {
    ClipboardChunk::send(getStream(), e.getDataObject());
  });
}

void ClientProxy1_6::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  // ignore if this clipboard is already clean
  if (m_clipboard[id].m_dirty) {
    // this clipboard is now clean
    m_clipboard[id].m_dirty = false;
    Clipboard::copy(&m_clipboard[id].m_clipboard, clipboard);

    std::string data = m_clipboard[id].m_clipboard.marshall();

    size_t size = data.size();
    LOG_DEBUG("sending clipboard %d to \"%s\"", id, getName().c_str());

    StreamChunker::sendClipboard(data, size, id, 0, m_events, this);
  }
}

bool ClientProxy1_6::recvClipboard()
{
  // parse message
  static std::string dataCached;
  ClipboardID id;
  uint32_t seq;

  if (auto r = ClipboardChunk::assemble(getStream(), dataCached, id, seq); r == TransferState::Started) {
    size_t size = ClipboardChunk::getExpectedSize();
    LOG_DEBUG("receiving clipboard %d size=%d", id, size);
  } else if (r == TransferState::Finished) {
    LOG(
        (CLOG_DEBUG "received client \"%s\" clipboard %d seqnum=%d, size=%d", getName().c_str(), id, seq,
         dataCached.size())
    );
    // save clipboard
    m_clipboard[id].m_clipboard.unmarshall(dataCached, 0);
    m_clipboard[id].m_sequenceNumber = seq;

    // notify
    auto *info = new ClipboardInfo;
    info->m_id = id;
    info->m_sequenceNumber = seq;
    m_events->addEvent(Event(EventTypes::ClipboardChanged, getEventTarget(), info));
  }

  return true;
}
