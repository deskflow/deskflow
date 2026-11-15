/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsFileDragPipe.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

//! Posted to the main thread when frames from the helper are waiting (see dispatch())
#define DESKFLOW_MSG_FILEDRAG (WM_APP + 0x0030)

/**
 * @brief deskflow-core's side of the file drag helper (deskflow-filedrag.exe)
 *
 * Starts the helper as the signed-in user, writes frames to it from a writer thread (the
 * main thread never blocks on the pipe) and reads its frames on a reader thread. Frames
 * are handed to the main thread through DESKFLOW_MSG_FILEDRAG and dispatch().
 */
class MSWindowsFileDragBridge
{
public:
  using Frame = deskflow::filedrag::pipe::Frame;
  using Handler = std::function<void(Frame, const std::string &)>;

  explicit MSWindowsFileDragBridge(bool primary);
  ~MSWindowsFileDragBridge();

  MSWindowsFileDragBridge(const MSWindowsFileDragBridge &) = delete;
  MSWindowsFileDragBridge &operator=(const MSWindowsFileDragBridge &) = delete;

  /**
   * @brief Starts the helper and waits until it is ready
   *
   * \p handler runs on the calling (main) thread from dispatch(). Returns false (and logs
   * why) when file drag cannot work here; nothing is left running then.
   */
  bool start(Handler handler);

  //! Asks the helper to quit and waits briefly for it
  void stop();

  //! Capability flags the helper reported, 0 when it is not running
  uint32_t capabilities() const
  {
    return m_running ? m_capabilities.load() : 0;
  }

  //! Queues a frame for the helper
  void send(Frame frame, std::string payload = {});

  /**
   * @brief Sends a frame that starts with a u32 token and waits for the helper's SyncAck
   *
   * Used where the helper must have acted before the core goes on (e.g. the catcher window
   * must be up before the cursor is moved onto it). Gives up after \p timeoutMs.
   */
  bool call(Frame frame, const std::string &payloadAfterToken, int timeoutMs = 500);

  //! Main thread: handles the frames that arrived
  void dispatch();

private:
  void readerLoop();
  void writerLoop();

  bool m_primary;
  DWORD m_mainThread = 0;
  Handler m_handler;
  deskflow::filedrag::pipe::HelperProcess m_process;
  std::atomic<bool> m_running{false};
  std::atomic<uint32_t> m_capabilities{0};

  std::thread m_reader;
  std::thread m_writer;

  std::mutex m_inMutex;
  std::deque<std::pair<Frame, std::string>> m_in;

  std::mutex m_outMutex;
  std::condition_variable m_outCond;
  std::deque<std::pair<Frame, std::string>> m_out;
  bool m_stopWriter = false;

  std::mutex m_syncMutex;
  std::condition_variable m_syncCond;
  uint32_t m_syncAcked = 0;
  uint32_t m_nextToken = 1;
  bool m_hello = false;
};
