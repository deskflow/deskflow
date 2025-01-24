/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_6.h"

#include "base/Log.h"
#include "base/TMethodEventJob.h"
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
  m_events->adoptHandler(
      m_events->forClipboard().clipboardSending(), this,
      new TMethodEventJob<ClientProxy1_6>(this, &ClientProxy1_6::handleClipboardSendingEvent)
  );
}

ClientProxy1_6::~ClientProxy1_6()
{
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
    LOG((CLOG_DEBUG "sending clipboard %d to \"%s\"", id, getName().c_str()));

    StreamChunker::sendClipboard(data, size, id, 0, m_events, this);
  }
}

void ClientProxy1_6::handleClipboardSendingEvent(const Event &event, void *)
{
  ClipboardChunk::send(getStream(), event.getDataObject());
}

bool ClientProxy1_6::recvClipboard()
{
  // parse message
  static std::string dataCached;
  ClipboardID id;
  uint32_t seq;

  int r = ClipboardChunk::assemble(getStream(), dataCached, id, seq);

  if (r == kStart) {
    size_t size = ClipboardChunk::getExpectedSize();
    LOG((CLOG_DEBUG "receiving clipboard %d size=%d", id, size));
  } else if (r == kFinish) {
    LOG(
        (CLOG_DEBUG "received client \"%s\" clipboard %d seqnum=%d, size=%d", getName().c_str(), id, seq,
         dataCached.size())
    );
    // save clipboard
    m_clipboard[id].m_clipboard.unmarshall(dataCached, 0);
    m_clipboard[id].m_sequenceNumber = seq;

    // notify
    ClipboardInfo *info = new ClipboardInfo;
    info->m_id = id;
    info->m_sequenceNumber = seq;
    m_events->addEvent(Event(m_events->forClipboard().clipboardChanged(), getEventTarget(), info));
  }

  return true;
}
