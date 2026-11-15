/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragTypes.h"

#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace deskflow::filedrag {

/**
 * @brief Builds the item list for the files and folders the user is dragging
 *
 * Each root becomes a top-level item named after it; folders are walked recursively
 * (symbolic links and junctions are not followed, unreadable entries are skipped).
 * Parents always come before their children. \p sources gets the local path of every
 * item, in the same order. Fails if there are more than kMaxItems items.
 */
bool scanItems(
    const std::vector<std::filesystem::path> &roots, ItemList &items, std::vector<std::filesystem::path> &sources,
    std::string *error = nullptr
);

/**
 * @brief Decides how many chunks may be unacknowledged
 *
 * Keeps the queue in front of the pointer short: the window grows while the round
 * trip of a chunk stays close to the quickest seen, and shrinks as soon as chunks start
 * waiting in buffers (round trip more than targetDelay above the quickest). Mouse and
 * key messages share the connection, so a short queue means they are never held up by
 * more than a few milliseconds of file data.
 */
class FlowControl
{
public:
  using Clock = std::chrono::steady_clock;

  explicit FlowControl(
      uint32_t initialWindow = 4, uint32_t minWindow = 1, uint32_t maxWindow = 16,
      std::chrono::microseconds targetDelay = std::chrono::milliseconds(4)
  );

  bool canSend() const
  {
    return inFlight() < m_window;
  }
  void onSent(uint32_t seq, Clock::time_point now);
  //! \p seq is cumulative: every chunk up to and including it was written
  void onAck(uint32_t seq, Clock::time_point now);

  uint32_t window() const
  {
    return m_window;
  }
  uint32_t inFlight() const
  {
    return static_cast<uint32_t>(m_sent.size());
  }
  std::chrono::microseconds minRtt() const
  {
    return m_minRtt;
  }
  uint32_t lastAcked() const
  {
    return m_lastAcked;
  }

private:
  struct Sent
  {
    uint32_t seq;
    Clock::time_point at;
  };

  uint32_t m_window;
  uint32_t m_minWindow;
  uint32_t m_maxWindow;
  std::chrono::microseconds m_targetDelay;
  std::deque<Sent> m_sent;
  std::chrono::microseconds m_minRtt = std::chrono::microseconds::max();
  Clock::time_point m_minRttAt;
  uint32_t m_lastAcked = 0;
  uint32_t m_lastSent = 0;
  uint32_t m_adjustAfter = 0;
};

/**
 * @brief Streams the files of one drag as FileStart / Chunk / FileEnd messages
 *
 * Files are read one chunk at a time (never a whole file in memory) and only while the
 * flow control window has room; acks open the window again. A file that cannot be
 * read, or that changed size since the item list was built, fails the drag with
 * ReadFailed.
 */
class FileDragSender
{
public:
  using Send = std::function<void(Message &&)>;
  using Clock = FlowControl::Clock;

  FileDragSender(uint32_t dragId, ItemList items, std::vector<std::filesystem::path> sources, Send send);

  //! Sends as much as the window allows
  void pump(Clock::time_point now = Clock::now());
  //! Handles an ack and sends more
  void onAck(uint32_t seq, Clock::time_point now = Clock::now());
  //! Stops sending (no message is sent)
  void stop();

  bool isFinished() const
  {
    return m_state == State::Done;
  }
  bool hasFailed() const
  {
    return m_state == State::Failed;
  }
  bool isAllSent() const
  {
    return m_state == State::AllSent || m_state == State::Done;
  }
  CancelReason failReason() const
  {
    return m_failReason;
  }
  uint64_t bytesSent() const
  {
    return m_bytesSent;
  }
  const FlowControl &flow() const
  {
    return m_flow;
  }
  uint32_t dragId() const
  {
    return m_dragId;
  }

private:
  enum class State
  {
    Sending,
    AllSent, ///< Everything is sent, waiting for the last acks
    Done,
    Failed,
    Stopped
  };

  bool sendNext(Clock::time_point now);
  void fail(CancelReason reason);
  bool openNext();

  uint32_t m_dragId;
  ItemList m_items;
  std::vector<std::filesystem::path> m_sources;
  Send m_send;
  FlowControl m_flow;
  State m_state = State::Sending;
  CancelReason m_failReason = CancelReason::Unknown;

  int64_t m_current = -1; ///< Item being sent
  bool m_fileOpen = false;
  uint64_t m_fileSent = 0;
  std::ifstream m_in;
  uint32_t m_seq = 0;
  uint64_t m_bytesSent = 0;
  std::string m_buffer;
};

} // namespace deskflow::filedrag
