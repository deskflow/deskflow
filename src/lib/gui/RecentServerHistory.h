/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace deskflow::gui {

// Address shortcuts only: this class never stores or grants certificate trust.
class RecentServerHistory
{
public:
  struct Entry
  {
    std::string address;
    int port = 24800;
    bool operator==(const Entry &) const = default;
  };
  static constexpr std::size_t MaxEntries = 5;

  const std::vector<Entry> &entries() const
  {
    return m_entries;
  }
  void load(const std::vector<Entry> &entries);
  void clear();
  void beginAttempt(const Entry &entry, bool tlsAndPeerChecksEnabled);
  void cancelAttempt();
  void disconnected();
  bool tlsEstablished();
  bool connected();

private:
  static std::optional<Entry> normalized(const Entry &entry);
  bool rememberIfReady();

  std::vector<Entry> m_entries;
  std::optional<Entry> m_attempt;
  bool m_tls = false;
  bool m_connected = false;
  bool m_recorded = false;
};

} // namespace deskflow::gui
