/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/ElectionState.h"

#include <cstdint>
#include <string>

namespace deskflow::coordination {

//! One decoded coordination-mesh message.
/*!
Wire format is newline-delimited JSON, byte-compatible with the legacy
external coordinator (docs/coordination/behavior-spec.md §2), so existing
operator tooling (kvmctl) keeps working unchanged.
*/
struct Message
{
  enum class Type
  {
    Invalid,
    Claim,
    Promote,
    Status
  };

  Type type = Type::Invalid;
  std::string name;
  std::string ip;
  std::string lan;
  int64_t seq = 0;
  std::string token;
};

namespace protocol {

//! Decode one line; returns Type::Invalid on malformed input.
Message decode(const std::string &line);

std::string encodeClaim(
    const std::string &name, const std::string &ip, const std::string &lan, int64_t seq, const std::string &token
);
std::string encodePromote(const std::string &token);
std::string encodeStatus(const std::string &token);

//! Status reply (legacy shape: role/server_ip/seq/last_switch/name).
std::string encodeStatusReply(
    Role role, const std::string &serverAddress, int64_t seq, double lastSwitchAt, const std::string &name
);

//! A decoded status reply.
struct StatusReply
{
  bool valid = false;
  Role role = Role::Init;
  std::string serverAddress;
  std::string name;
};

StatusReply decodeStatusReply(const std::string &line);

} // namespace protocol

} // namespace deskflow::coordination
