/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "RecentServerHistory.h"

#include <algorithm>

namespace deskflow::gui {

std::optional<RecentServerHistory::Entry> RecentServerHistory::normalized(const Entry &entry)
{
  if (entry.address.empty() || entry.address.size() > 1024 || entry.port < 1 || entry.port > 65535)
    return std::nullopt;

  std::string result;
  std::size_t start = 0;
  int count = 0;
  while (start <= entry.address.size()) {
    const auto end = entry.address.find(',', start);
    auto token = entry.address.substr(start, end == std::string::npos ? end : end - start);
    const auto first = token.find_first_not_of(' ');
    if (first == std::string::npos)
      return std::nullopt;
    token = token.substr(first, token.find_last_not_of(' ') - first + 1);
    if (token.size() > 255 || token.front() == '-' || ++count > 8)
      return std::nullopt;
    // Keep stored menu text bounded and free of control characters, URLs and markup.
    // Manual connection parsing remains the responsibility of the existing core.
    for (const unsigned char c : token) {
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.' ||
            c == '_' || c == ':' || c == '[' || c == ']' || c == '%'))
        return std::nullopt;
    }
    if (!result.empty())
      result += ',';
    result += token;
    if (end == std::string::npos)
      break;
    start = end + 1;
  }
  return Entry{result, entry.port};
}

void RecentServerHistory::load(const std::vector<Entry> &entries)
{
  m_entries.clear();
  for (const auto &entry : entries) {
    const auto value = normalized(entry);
    if (value && std::find(m_entries.begin(), m_entries.end(), *value) == m_entries.end())
      m_entries.push_back(*value);
    if (m_entries.size() == MaxEntries)
      break;
  }
}

void RecentServerHistory::clear()
{
  m_entries.clear();
  // Repeated IPC status messages must not undo an explicit clear.
  m_recorded = true;
}

void RecentServerHistory::beginAttempt(const Entry &entry, bool tlsAndPeerChecksEnabled)
{
  cancelAttempt();
  if (tlsAndPeerChecksEnabled)
    m_attempt = normalized(entry);
}

void RecentServerHistory::cancelAttempt()
{
  m_attempt.reset();
  disconnected();
}

void RecentServerHistory::disconnected()
{
  m_tls = false;
  m_connected = false;
  m_recorded = false;
}

bool RecentServerHistory::tlsEstablished()
{
  m_tls = true;
  return rememberIfReady();
}

bool RecentServerHistory::connected()
{
  m_connected = true;
  return rememberIfReady();
}

bool RecentServerHistory::rememberIfReady()
{
  if (!m_attempt || !m_tls || !m_connected || m_recorded)
    return false;
  m_recorded = true;
  const auto previous = m_entries;
  std::erase(m_entries, *m_attempt);
  m_entries.insert(m_entries.begin(), *m_attempt);
  if (m_entries.size() > MaxEntries)
    m_entries.resize(MaxEntries);
  return previous != m_entries;
}

} // namespace deskflow::gui
