/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_8.h"

//! Proxy for client implementing protocol version 1.9
/*!
1.9 adds dragging files and folders between the server and a client (kMsgDFileDrag*).
The client speaks first with kMsgDFileDragCaps once the handshake is done; this proxy
answers with the server's flags and only then sends file drag messages, and only the
ones the client said it understands: server to client drags to a client that can take
them, and the client to server messages (watch, hand-off, acks) to a client that set
kCapSource and kCapReverse.
*/
class ClientProxy1_9 : public ClientProxy1_8
{
public:
  ClientProxy1_9(const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events);
  ~ClientProxy1_9() override = default;

  bool fileDragAvailable() const override;
  bool fileDragReverseAvailable() const override;
  void fileDragSend(const deskflow::filedrag::Message &message) override;

protected:
  bool parseMessage(const uint8_t *code) override;

private:
  uint32_t m_peerCaps = deskflow::filedrag::kCapNone;
  bool m_sentCaps = false;
};
