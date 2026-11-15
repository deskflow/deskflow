/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ServerProxy1_9.h"

#include "base/Log.h"
#include "client/Client.h"
#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragProtocol.h"
#include "deskflow/ProtocolTypes.h"

#include <cstring>

using namespace deskflow::filedrag;

ServerProxy1_9::ServerProxy1_9(Client *client, deskflow::IStream *stream, IEventQueue *events)
    : ServerProxy1_8(client, stream, events)
{
}

void ServerProxy1_9::fileDragSend(const Message &message)
{
  if (!m_sentCaps) {
    return;
  }
  switch (message.type) {
    using enum Message::Type;
  case Offer:
  case Icon:
  case Begin:
  case FileStart:
  case Chunk:
  case FileEnd:
    // client to server drags: only to a server that said it takes them (older ones hang up)
    if (!reverseDragAvailable(m_peerCaps, getClient()->fileDragCapabilities())) {
      return;
    }
    break;
  default:
    break;
  }
  LOG_VERBOSE(
      "send file drag %s drag=%u seq=%u reason=%d", messageTypeName(message.type), message.dragId, message.seq,
      static_cast<int>(message.reason)
  );
  writeMessage(getStream(), message);
}

ServerProxy::ConnectionResult ServerProxy1_9::parseHandshakeMessage(const uint8_t *code)
{
  const bool completesHandshake = memcmp(code, kMsgDSetOptions, 4) == 0;
  const auto result = ServerProxy1_8::parseHandshakeMessage(code);
  if (completesHandshake && result == ConnectionResult::Okay && !m_sentCaps) {
    // speak first: a server that uses 1.9 for something else just ignores this
    m_sentCaps = true;
    const auto caps = getClient()->fileDragCapabilities();
    LOG_INFO("file drag capabilities sent to the server: 0x%x", caps);
    writeMessage(getStream(), Message::caps(caps));
  }
  return result;
}

ServerProxy::ConnectionResult ServerProxy1_9::parseMessage(const uint8_t *code)
{
  if (isFileDragCode(code)) {
    return fileDragMessage(code);
  }
  return ServerProxy1_8::parseMessage(code);
}

ServerProxy::ConnectionResult ServerProxy1_9::fileDragMessage(const uint8_t *code)
{
  Message message;
  std::string error;
  if (!readMessage(getStream(), code, message, &error)) {
    if (message.type == Message::Type::Begin && message.dragId != 0) {
      // well formed but refused (unsafe path, bad list): cancel just this drag
      LOG_WARN("refusing file drag %u: %s", message.dragId, error.c_str());
      fileDragSend(Message::cancel(message.dragId, CancelReason::Invalid));
      return ConnectionResult::Okay;
    }
    LOG_ERR("bad file drag message from server: %s", error.c_str());
    return ConnectionResult::Unknown;
  }

  switch (message.type) {
    using enum Message::Type;
  case Caps:
    m_peerCaps = message.flags;
    LOG_INFO(
        "server file drag capabilities: 0x%x (file drags from this computer to the server: %s)", m_peerCaps,
        reverseDragAvailable(m_peerCaps, getClient()->fileDragCapabilities()) ? "yes" : "no"
    );
    return ConnectionResult::Okay;

  case Chunk:
    // file data keeps the connection alive as much as a keep-alive does
    resetKeepAliveAlarm();
    [[fallthrough]];
  case Icon:
  case Begin:
  case FileStart:
  case FileEnd:
  case Cancel:
    getClient()->fileDragReceive(message);
    return ConnectionResult::Okay;

  case Ack:
    resetKeepAliveAlarm();
    [[fallthrough]];
  case Watch:
  case HandOff:
  case Dropped:
  case Finished:
    // client to server drags (a server only sends these to a client that set kCapReverse)
    getClient()->fileDragReceive(message);
    return ConnectionResult::Okay;

  default:
    LOG_WARN("unexpected file drag %s from server", messageTypeName(message.type));
    return ConnectionResult::Okay;
  }
}
