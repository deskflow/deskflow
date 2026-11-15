/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileDragSender.h"

#include "deskflow/FileDragCodec.h"

#include <algorithm>
#include <set>
#include <system_error>

namespace fs = std::filesystem;

namespace deskflow::filedrag {

namespace {

std::string toUtf8(const fs::path &p)
{
  const auto u8 = p.generic_u8string();
  return std::string(u8.begin(), u8.end());
}

void setError(std::string *error, const std::string &message)
{
  if (error != nullptr) {
    *error = message;
  }
}

//! Names straight from a file system can hold bytes the protocol refuses (Linux allows
//! control characters); replace those so the receiver accepts the list.
std::string protocolSafeComponent(std::string name)
{
  for (auto &c : name) {
    if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f || c == '/' || c == '\\') {
      c = '_';
    }
  }
  if (name.empty() || name == "." || name == "..") {
    name = "_";
  }
  return name;
}

std::string protocolSafePath(const fs::path &relative)
{
  std::string out;
  for (const auto &part : relative) {
    if (!out.empty()) {
      out.push_back('/');
    }
    out += protocolSafeComponent(toUtf8(part));
  }
  return out;
}

} // namespace

//
// scanItems
//

bool scanItems(const std::vector<fs::path> &roots, ItemList &items, std::vector<fs::path> &sources, std::string *error)
{
  items.clear();
  sources.clear();
  std::set<std::string> topLevel;

  auto add = [&](std::string path, uint64_t size, bool isDirectory, const fs::path &source) {
    if (items.size() >= kMaxItems) {
      setError(error, "too many files (more than " + std::to_string(kMaxItems) + ")");
      return false;
    }
    if (!isSafeRelativePath(path)) {
      setError(error, "path too long or not allowed: " + path);
      return false;
    }
    items.push_back(Item{std::move(path), isDirectory ? 0 : size, isDirectory});
    sources.push_back(source);
    return true;
  };

  for (const auto &root : roots) {
    std::error_code ec;
    const auto status = fs::symlink_status(root, ec);
    if (ec) {
      continue;
    }
    const bool isLink = fs::is_symlink(status);
    const bool isDir = fs::is_directory(isLink ? fs::status(root, ec) : status);
    const bool isFile = fs::is_regular_file(isLink ? fs::status(root, ec) : status);
    if (!isDir && !isFile) {
      continue;
    }

    auto rootName = protocolSafeComponent(toUtf8(root.filename()));
    if (root.filename().empty()) {
      rootName = protocolSafeComponent(toUtf8(root.parent_path().filename()));
    }
    rootName = uniqueName(rootName, isDir, [&](const std::string &n) { return topLevel.contains(nameKey(n)); });
    topLevel.insert(nameKey(rootName));

    if (isFile) {
      const auto size = fs::file_size(root, ec);
      if (ec) {
        continue;
      }
      if (!add(rootName, size, false, root)) {
        return false;
      }
      continue;
    }

    if (!add(rootName, 0, true, root)) {
      return false;
    }
    if (isLink) {
      continue; // never follow linked folders: they can loop
    }

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
      const auto &entry = *it;
      std::error_code entryError;
      const auto entryStatus = entry.symlink_status(entryError);
      if (!entryError) {
        const auto relative = entry.path().lexically_relative(root);
        const auto path = rootName + "/" + protocolSafePath(relative);
        if (fs::is_directory(entryStatus)) {
          if (!add(path, 0, true, entry.path())) {
            return false;
          }
        } else if (fs::is_regular_file(entryStatus) ||
                   (fs::is_symlink(entryStatus) && entry.is_regular_file(entryError))) {
          const auto size = entry.file_size(entryError);
          if (!entryError && !add(path, size, false, entry.path())) {
            return false;
          }
        }
        // linked folders, devices, sockets and pipes are skipped
      }
      it.increment(ec);
    }
  }

  if (items.empty()) {
    setError(error, "nothing to send");
    return false;
  }
  return validateItems(items, error);
}

//
// FlowControl
//

FlowControl::FlowControl(
    uint32_t initialWindow, uint32_t minWindow, uint32_t maxWindow, std::chrono::microseconds targetDelay
)
    : m_window(std::clamp(initialWindow, minWindow, maxWindow)),
      m_minWindow(minWindow),
      m_maxWindow(maxWindow),
      m_targetDelay(targetDelay)
{
}

void FlowControl::onSent(uint32_t seq, Clock::time_point now)
{
  m_sent.push_back(Sent{seq, now});
  m_lastSent = seq;
}

void FlowControl::onAck(uint32_t seq, Clock::time_point now)
{
  if (seq <= m_lastAcked) {
    return;
  }

  bool sampled = false;
  Clock::time_point sentAt;
  while (!m_sent.empty() && m_sent.front().seq <= seq) {
    sentAt = m_sent.front().at;
    sampled = true;
    m_sent.pop_front();
  }
  m_lastAcked = seq;
  if (!sampled) {
    return;
  }

  const auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(now - sentAt);
  // forget the quickest round trip now and then in case the route changed
  if (rtt < m_minRtt || now - m_minRttAt > std::chrono::seconds(10)) {
    m_minRtt = rtt;
    m_minRttAt = now;
  }

  // adjust at most once per flight of chunks
  if (seq < m_adjustAfter) {
    return;
  }
  const auto queueDelay = rtt - m_minRtt;
  if (queueDelay > m_targetDelay) {
    const uint32_t shrink = std::max<uint32_t>(1, m_window / 4);
    m_window = m_window > m_minWindow + shrink ? m_window - shrink : m_minWindow;
  } else if (queueDelay * 2 <= m_targetDelay && m_window < m_maxWindow) {
    ++m_window;
  }
  m_adjustAfter = m_lastSent;
}

//
// FileDragSender
//

FileDragSender::FileDragSender(uint32_t dragId, ItemList items, std::vector<fs::path> sources, Send send)
    : m_dragId(dragId),
      m_items(std::move(items)),
      m_sources(std::move(sources)),
      m_send(std::move(send))
{
  m_buffer.resize(kChunkSize);
}

void FileDragSender::pump(Clock::time_point now)
{
  while (m_state == State::Sending && m_flow.canSend()) {
    if (!sendNext(now)) {
      break;
    }
  }
  if (m_state == State::AllSent && m_flow.inFlight() == 0) {
    m_state = State::Done;
  }
}

void FileDragSender::onAck(uint32_t seq, Clock::time_point now)
{
  if (m_state == State::Failed || m_state == State::Stopped) {
    return;
  }
  m_flow.onAck(seq, now);
  pump(now);
}

void FileDragSender::stop()
{
  if (m_state != State::Failed) {
    m_state = State::Stopped;
  }
  m_in.close();
  m_fileOpen = false;
}

void FileDragSender::fail(CancelReason reason)
{
  m_state = State::Failed;
  m_failReason = reason;
  m_in.close();
  m_fileOpen = false;
}

bool FileDragSender::openNext()
{
  for (auto next = static_cast<size_t>(m_current + 1); next < m_items.size(); ++next) {
    if (m_items[next].isDirectory) {
      continue;
    }
    m_current = static_cast<int64_t>(next);
    m_in.clear();
    m_in.open(m_sources[next], std::ios::binary);
    if (!m_in) {
      fail(CancelReason::ReadFailed);
      return false;
    }
    m_fileOpen = true;
    m_fileSent = 0;

    Message start;
    start.type = Message::Type::FileStart;
    start.dragId = m_dragId;
    start.index = static_cast<uint32_t>(next);
    start.size = m_items[next].size;
    m_send(std::move(start));
    return true;
  }
  m_current = static_cast<int64_t>(m_items.size());
  m_state = State::AllSent;
  return false;
}

bool FileDragSender::sendNext(Clock::time_point now)
{
  if (!m_fileOpen && !openNext()) {
    return false;
  }

  const auto index = static_cast<uint32_t>(m_current);
  const auto size = m_items[index].size;
  const auto remaining = size - m_fileSent;

  if (remaining > 0) {
    const auto wanted = static_cast<std::streamsize>(std::min<uint64_t>(kChunkSize, remaining));
    m_in.read(m_buffer.data(), wanted);
    if (m_in.gcount() != wanted) {
      // shorter than when the list was built: the receiver would get a wrong file
      fail(CancelReason::ReadFailed);
      return false;
    }
    Message chunk;
    chunk.type = Message::Type::Chunk;
    chunk.dragId = m_dragId;
    chunk.index = index;
    chunk.seq = ++m_seq;
    chunk.data.assign(m_buffer.data(), static_cast<size_t>(wanted));
    m_fileSent += static_cast<uint64_t>(wanted);
    m_bytesSent += static_cast<uint64_t>(wanted);
    m_flow.onSent(chunk.seq, now);
    m_send(std::move(chunk));
  }

  if (m_fileSent == size) {
    // longer than when the list was built is just as wrong
    if (m_in.peek() != std::ifstream::traits_type::eof()) {
      fail(CancelReason::ReadFailed);
      return false;
    }
    m_in.close();
    m_fileOpen = false;
    Message end;
    end.type = Message::Type::FileEnd;
    end.dragId = m_dragId;
    end.index = index;
    m_send(std::move(end));
  }
  return true;
}

} // namespace deskflow::filedrag
