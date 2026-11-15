/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_9.h"

#include "base/Log.h"
#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragProtocol.h"
#include "server/Server.h"

using namespace deskflow::filedrag;

ClientProxy1_9::ClientProxy1_9(
    const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events
)
    : ClientProxy1_8(name, adoptedStream, server, events)
{
  // nothing is sent here: the client announces its file drag support first
}

bool ClientProxy1_9::fileDragAvailable() const
{
  return (m_peerCaps & kCapDestination) != 0 && m_sentCaps && (m_server->fileDragLocalCapabilities() & kCapSource) != 0;
}

bool ClientProxy1_9::fileDragReverseAvailable() const
{
  return m_sentCaps && reverseDragAvailable(m_server->fileDragLocalCapabilities(), m_peerCaps);
}

void ClientProxy1_9::fileDragSend(const Message &message)
{
  bool allowed = false;
  switch (message.type) {
    using enum Message::Type;
  case Caps:
    allowed = true;
    break;
  case Watch:
  case HandOff:
  case Ack:
  case Dropped:
  case Finished:
    // only a client that sends file drags here gets these (older ones would hang up)
    allowed = fileDragReverseAvailable();
    break;
  case Cancel:
    allowed = fileDragAvailable() || fileDragReverseAvailable();
    break;
  default:
    allowed = fileDragAvailable();
    break;
  }
  if (!allowed) {
    return;
  }
  LOG_VERBOSE(
      "send file drag %s to \"%s\" drag=%u index=%u seq=%u", messageTypeName(message.type), getName().c_str(),
      message.dragId, message.index, message.seq
  );
  writeMessage(getStream(), message);
}

bool ClientProxy1_9::parseMessage(const uint8_t *code)
{
  if (!isFileDragCode(code)) {
    return ClientProxy1_8::parseMessage(code);
  }

  Message message;
  std::string error;
  if (!readMessage(getStream(), code, message, &error)) {
    if (message.type == Message::Type::Begin && message.dragId != 0) {
      // well formed but refused (unsafe path, bad list): cancel just this drag
      LOG_WARN("refusing file drag %u from \"%s\": %s", message.dragId, getName().c_str(), error.c_str());
      fileDragSend(Message::cancel(message.dragId, CancelReason::Invalid));
      m_server->onFileDragMessage(this, Message::cancel(message.dragId, CancelReason::Invalid));
      return true;
    }
    LOG_ERR("bad file drag message from \"%s\": %s", getName().c_str(), error.c_str());
    return false;
  }

  switch (message.type) {
    using enum Message::Type;
  case Caps: {
    m_peerCaps = message.flags;
    if (!m_sentCaps) {
      m_sentCaps = true;
      writeMessage(getStream(), Message::caps(m_server->fileDragLocalCapabilities()));
    }
    // once per connection; says at a glance which way files can be dragged
    LOG_INFO(
        "client \"%s\" %s file drags from here and %s send file drags here (capabilities 0x%x)", getName().c_str(),
        (m_peerCaps & kCapDestination) != 0 ? "can receive" : "cannot receive",
        fileDragReverseAvailable() ? "can" : "cannot", m_peerCaps
    );
    return true;
  }

  case Ack:
  case Cancel:
  case Dropped:
  case Finished:
    m_server->onFileDragMessage(this, message);
    return true;

  case Offer:
  case Icon:
  case Begin:
  case FileStart:
  case Chunk:
  case FileEnd:
    if (!fileDragReverseAvailable()) {
      LOG_WARN("unexpected file drag %s from \"%s\"", messageTypeName(message.type), getName().c_str());
      return true;
    }
    m_server->onFileDragMessage(this, message);
    return true;

  default:
    // watch and hand-off only travel from the server to the client
    LOG_WARN("unexpected file drag %s from \"%s\"", messageTypeName(message.type), getName().c_str());
    return true;
  }
}
