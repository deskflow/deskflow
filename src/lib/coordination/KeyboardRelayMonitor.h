/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/CoordinationProtocol.h"
#include "deskflow/KeyTypes.h"

#include <functional>
#include <memory>

namespace deskflow::coordination {

//! Fleet keyboard relay: forward local keys when the cursor is on another peer.
//! During client epoch, the query returns true when the key should reach the
//! local OS (pass-through) and false when it should be forwarded to the server.
class IKeyboardRelayMonitor
{
public:
  using RelayPassThroughQuery = std::function<bool()>;
  //! Deprecated alias; returns true when the key should reach the local OS.
  using CursorOnSelfQuery = RelayPassThroughQuery;
  using KeyForwardSend = std::function<void(
      Message::KeyPhase phase, KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang
  )>;

  virtual ~IKeyboardRelayMonitor() = default;

  virtual bool start(RelayPassThroughQuery passThrough, KeyForwardSend send) = 0;
  virtual void stop() = 0;
};

std::unique_ptr<IKeyboardRelayMonitor> createKeyboardRelayMonitor();

} // namespace deskflow::coordination
