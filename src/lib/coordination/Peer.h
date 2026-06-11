/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <vector>

namespace deskflow::coordination {

//! A coordination-mesh member.
/*!
A peer is reachable at a primary address (\c ip, typically stable / VPN)
and optionally a LAN address (\c lan) preferred when reachable.
*/
struct Peer
{
  std::string name;
  std::string ip;
  std::string lan;

  bool hasAddress(const std::string &address) const
  {
    return !address.empty() && (address == ip || address == lan);
  }
};

using PeerList = std::vector<Peer>;

//! Parse a peers setting string: comma-separated `name=ip[|lan]` entries.
/*!
Whitespace around entries is ignored; malformed entries are skipped.
Example: `"macbookpro=100.75.218.20|macbookpro.local, tiny11=100.90.248.22"`.
*/
PeerList parsePeerList(const std::string &setting);

} // namespace deskflow::coordination
