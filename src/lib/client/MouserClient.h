/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Mouser client (fork extension): forwards kMsgDMouserData payload lines
 * from the Deskflow server into the local Mouser instance's remote-device
 * port (loopback). Owns a worker thread so a slow or absent Mouser can
 * never stall the input-processing path.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

class MouserClient
{
public:
  MouserClient(int port, std::string token);
  MouserClient(MouserClient const &) = delete;
  MouserClient(MouserClient &&) = delete;
  ~MouserClient();

  MouserClient &operator=(MouserClient const &) = delete;
  MouserClient &operator=(MouserClient &&) = delete;

  //! Thread-safe: enqueue one Mouser-protocol JSON line for delivery.
  void deliver(const std::string &line);

private:
  void workerLoop();
  bool ensureConnected();
  bool sendLine(const std::string &line);
  void drainReplies();
  void disconnect();

  int m_port;
  std::string m_token;

  std::thread m_thread;
  std::atomic<bool> m_running{true};
  std::mutex m_mutex;
  std::condition_variable m_wake;
  std::deque<std::string> m_queue;

  int m_fd = -1;
};
