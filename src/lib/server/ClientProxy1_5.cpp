/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_5.h"

#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/FileChunk.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/StreamChunker.h"
#include "io/IStream.h"
#include "server/Server.h"

#include <sstream>

//
// ClientProxy1_5
//

ClientProxy1_5::ClientProxy1_5(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_4(name, stream, server, events),
      m_events(events)
{

  m_events->adoptHandler(
      m_events->forFile().keepAlive(), this,
      new TMethodEventJob<ClientProxy1_3>(this, &ClientProxy1_3::handleKeepAlive, NULL)
  );
}

ClientProxy1_5::~ClientProxy1_5()
{
  m_events->removeHandler(m_events->forFile().keepAlive(), this);
}

void ClientProxy1_5::sendDragInfo(uint32_t fileCount, const char *info, size_t size)
{
  std::string data(info, size);

  ProtocolUtil::writef(getStream(), kMsgDDragInfo, fileCount, &data);
}

void ClientProxy1_5::fileChunkSending(uint8_t mark, char *data, size_t dataSize)
{
  FileChunk::send(getStream(), mark, data, dataSize);
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

void ClientProxy1_5::fileChunkReceived()
{
  Server *server = getServer();
  int result = FileChunk::assemble(getStream(), server->getReceivedFileData(), server->getExpectedFileSize());

  if (result == kFinish) {
    m_events->addEvent(Event(m_events->forFile().fileRecieveCompleted(), server));
  } else if (result == kStart) {
    if (server->getFakeDragFileList().size() > 0) {
      std::string filename = server->getFakeDragFileList().at(0).getFilename();
      LOG((CLOG_DEBUG "start receiving %s", filename.c_str()));
    }
  }
}

void ClientProxy1_5::dragInfoReceived()
{
  // parse
  uint32_t fileNum = 0;
  std::string content;
  ProtocolUtil::readf(getStream(), kMsgDDragInfo + 4, &fileNum, &content);

  m_server->dragInfoReceived(fileNum, content);
}
