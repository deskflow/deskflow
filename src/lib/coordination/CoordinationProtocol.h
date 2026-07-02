/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/ElectionState.h"
#include "coordination/FleetState.h"

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
    Status,
    Cursor,
    KeyFwd,
    Hello,
    Fleet
  };

  enum class KeyPhase
  {
    Down,
    Up,
    Repeat
  };

  Type type = Type::Invalid;
  std::string name;
  std::string ip;
  std::string lan;
  int64_t seq = 0;
  std::string token;
  // cursor: host screen under the fleet cursor (server → peers)
  std::string host;
  // keyfwd: keyboard relay (peer → server)
  KeyPhase keyPhase = KeyPhase::Down;
  uint16_t keyId = 0;
  uint16_t keyMask = 0;
  uint16_t keyButton = 0;
  std::string keyLang;
  // mesh v2 hello
  int meshVersion = 0;
  // mesh v2 fleet fragment (decoded from `fleet` messages)
  FleetFragment fleet;
};

namespace protocol {

//! Decode one line; returns Type::Invalid on malformed input.
Message decode(const std::string &line);

std::string encodeClaim(
    const std::string &name, const std::string &ip, const std::string &lan, int64_t seq, const std::string &token
);
std::string encodePromote(const std::string &token);
std::string encodeStatus(const std::string &token);

std::string encodeCursor(const std::string &host, int64_t seq, const std::string &token);

std::string encodeKeyFwd(
    const std::string &from, Message::KeyPhase phase, uint16_t id, uint16_t mask, uint16_t button,
    const std::string &lang, const std::string &token
);

std::string encodeHello(int meshVersion, const std::string &name, const std::string &token);
std::string encodeFleet(const FleetFragment &fragment, const std::string &token);

//! Build a FleetFragment from a decoded fleet message.
FleetFragment fleetFragmentFromMessage(const Message &message);

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
