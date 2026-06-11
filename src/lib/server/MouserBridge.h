/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Mouser bridge (fork extension): loopback TCP listener the local Mouser
 * instance connects to. Receives Mouser-protocol JSON lines (device
 * connect / HID++ events / disconnect) and posts them onto the event loop
 * for the Server to relay to the active client via kMsgDMouserData; pushes
 * focus notifications back so Mouser knows when to forward vs. act locally.
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class IEventQueue;

//! Event data for EventTypes::ServerMouserBridgeLine.
class MouserBridgeLineData : public EventData
{
public:
  explicit MouserBridgeLineData(std::string line) : m_line(std::move(line))
  {
    // do nothing
  }
  ~MouserBridgeLineData() override = default;

  const std::string &line() const
  {
    return m_line;
  }

private:
  std::string m_line;
};

class MouserBridge
{
public:
  MouserBridge(IEventQueue *events, void *eventTarget, int port, std::string token);
  MouserBridge(MouserBridge const &) = delete;
  MouserBridge(MouserBridge &&) = delete;
  ~MouserBridge();

  MouserBridge &operator=(MouserBridge const &) = delete;
  MouserBridge &operator=(MouserBridge &&) = delete;

  //! Bind the loopback listener and start the accept/read thread.
  bool start();
  void stop();

  //! Thread-safe: push a focus notification to the connected Mouser
  //! instance (no-op when none is connected).
  void notifyFocus(const std::string &screenName, bool isLocal);

private:
  void serveLoop();
  void handleClient(int clientFd);
  bool authenticate(int clientFd, std::string &carry);
  bool readLine(int clientFd, std::string &carry, std::string &line) const;
  bool sendLine(int clientFd, const std::string &line);
  void closeFd(int &fd) const;

  IEventQueue *m_events;
  void *m_eventTarget;
  int m_port;
  std::string m_token;

  std::thread m_thread;
  std::atomic<bool> m_running{false};
  int m_listenFd = -1;
  std::mutex m_clientMutex;
  int m_clientFd = -1;

  // Remembered focus so a (re)connecting Mouser instance is brought up to
  // date immediately after its hello.
  std::mutex m_focusMutex;
  std::string m_focusLine;
};
