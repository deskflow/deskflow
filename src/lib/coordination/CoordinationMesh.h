/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/CoordinationProtocol.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace deskflow::coordination {

//! Coordination-mesh TCP transport.
/*!
Accepts inbound newline-JSON messages on the coordination port and sends
one-shot messages to peer addresses. Mirrors the legacy coordinator's
connection model (one TCP connect per send, no persistent mesh channel)
so it is wire-compatible with kvmctl.

Threading: a dedicated accept thread receives; sends happen on the
caller's thread (short-lived blocking connects with a small timeout).
The receive callback is invoked on the accept thread -- the Coordinator
marshals it onto the event-queue thread.
*/
class CoordinationMesh
{
public:
  //! ``reply`` writes a status response back on the same connection.
  using Receiver = std::function<void(const Message &message, const std::function<void(const std::string &)> &reply)>;

  CoordinationMesh(int port, std::string token, Receiver receiver);
  CoordinationMesh(const CoordinationMesh &) = delete;
  CoordinationMesh &operator=(const CoordinationMesh &) = delete;
  ~CoordinationMesh();

  bool start();
  void stop();

  //! Fire-and-forget send to ``host:port`` (connect timeout bounded).
  void sendTo(const std::string &host, const std::string &line);

  //! Connect-probe a peer's coordination port (LAN-first reachability).
  bool probe(const std::string &host, int timeoutMs);

  //! Connect-probe the local deskflow transport port (server wedge check).
  bool probeDeskflowPort(int port, int timeoutMs);

  //! Send one line and read one reply line (status query); empty on failure.
  std::string query(const std::string &host, const std::string &line);

  //! True when ``token`` is unset, or the message carries the right token.
  bool tokenOk(const Message &message) const;

private:
  void serveLoop();
  void handleClient(int clientFd);

  int m_port;
  std::string m_token;
  Receiver m_receiver;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  int m_listenFd = -1;
};

} // namespace deskflow::coordination
