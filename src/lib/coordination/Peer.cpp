/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/Peer.h"

namespace deskflow::coordination {

namespace {

std::string trimmed(const std::string &value)
{
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

} // namespace

PeerList parsePeerList(const std::string &setting)
{
  PeerList peers;
  std::string::size_type start = 0;
  while (start <= setting.size()) {
    auto end = setting.find(',', start);
    if (end == std::string::npos) {
      end = setting.size();
    }
    const std::string entry = trimmed(setting.substr(start, end - start));
    start = end + 1;
    if (entry.empty()) {
      continue;
    }

    const auto equals = entry.find('=');
    if (equals == std::string::npos || equals == 0 || equals == entry.size() - 1) {
      continue; // malformed: no name or no address
    }

    Peer peer;
    peer.name = trimmed(entry.substr(0, equals));
    std::string addresses = entry.substr(equals + 1);
    const auto pipe = addresses.find('|');
    if (pipe == std::string::npos) {
      peer.ip = trimmed(addresses);
    } else {
      peer.ip = trimmed(addresses.substr(0, pipe));
      peer.lan = trimmed(addresses.substr(pipe + 1));
    }
    if (peer.name.empty() || peer.ip.empty()) {
      continue;
    }
    if (peer.lan.empty()) {
      peer.lan = peer.ip;
    }
    peers.push_back(std::move(peer));
  }
  return peers;
}

} // namespace deskflow::coordination
