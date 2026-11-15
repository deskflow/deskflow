/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "client/ServerProxy1_8.h"

//! Proxy for server implementing protocol version 1.9
/*!
1.9 adds dragging files and folders between the server and this client. Once the
handshake is complete the client announces what it can do (kMsgDFileDragCaps); the server
only sends file drag messages after that, and this proxy only sends the client to server
ones (offer, items, data) to a server that said it takes them.
*/
class ServerProxy1_9 : public ServerProxy1_8
{
public:
  ServerProxy1_9(Client *client, deskflow::IStream *stream, IEventQueue *events);
  ~ServerProxy1_9() override = default;

  void fileDragSend(const deskflow::filedrag::Message &message) override;

protected:
  ConnectionResult parseHandshakeMessage(const uint8_t *code) override;
  ConnectionResult parseMessage(const uint8_t *code) override;

private:
  ConnectionResult fileDragMessage(const uint8_t *code);

  uint32_t m_peerCaps = deskflow::filedrag::kCapNone;
  bool m_sentCaps = false;
};
