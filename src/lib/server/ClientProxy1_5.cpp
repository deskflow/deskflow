/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_5.h"

#include "deskflow/ProtocolUtil.h"
#include "deskflow/StreamChunker.h"
#include "io/IStream.h"
#include "server/Server.h"

#include <cstring>

//
// ClientProxy1_5
//

ClientProxy1_5::ClientProxy1_5(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_4(name, stream, server, events)
{
  // do nothing
}

void ClientProxy1_5::sendDragInfo(uint32_t fileCount, const char *info, size_t size)
{
  // do nothing
}

void ClientProxy1_5::fileChunkSending(uint8_t mark, char *data, size_t dataSize)
{
  // do nothing
}

bool ClientProxy1_5::parseMessage(const uint8_t *code)
{
  if (memcmp(code, kMsgDFileTransfer, 4) == 0) {
    fileChunkReceived();
  } else if (memcmp(code, kMsgDDragInfo, 4) == 0) {
    dragInfoReceived();
  } else {
    return ClientProxy1_4::parseMessage(code);
  }

  return true;
}

void ClientProxy1_5::fileChunkReceived() const
{
  // File drag-and-drop is deprecated and no longer implemented, but a foreign or
  // older client (e.g. Synergy/Barrier) may still send these messages. Read and
  // discard the body ("%1i%s": a 1-byte mark and a length-prefixed chunk) so the
  // message stream stays framed. Leaving it unread would misparse the body as
  // the next message code and drop a burst of buffered input.
  uint8_t mark = 0;
  std::string content;
  ProtocolUtil::readf(getStream(), kMsgDFileTransfer + 4, &mark, &content);
}

void ClientProxy1_5::dragInfoReceived() const
{
  // As above: consume the deprecated drag-info body ("%2i%s": a 2-byte file
  // count and a length-prefixed path list) to keep the stream framed.
  uint16_t fileCount = 0;
  std::string content;
  ProtocolUtil::readf(getStream(), kMsgDDragInfo + 4, &fileCount, &content);
}
