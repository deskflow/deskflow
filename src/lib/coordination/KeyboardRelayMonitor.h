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
//! During client epoch, \c cursorOnSelf returns true when the shared cursor is
//! on this machine; the monitor passes keys to the local OS. When false, keys
//! are forwarded to the server and swallowed.
class IKeyboardRelayMonitor
{
public:
  using CursorOnSelfQuery = std::function<bool()>;
  using KeyForwardSend = std::function<void(
      Message::KeyPhase phase, KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang
  )>;

  virtual ~IKeyboardRelayMonitor() = default;

  virtual bool start(CursorOnSelfQuery cursorOnSelf, KeyForwardSend send) = 0;
  virtual void stop() = 0;
};

std::unique_ptr<IKeyboardRelayMonitor> createKeyboardRelayMonitor();

} // namespace deskflow::coordination
