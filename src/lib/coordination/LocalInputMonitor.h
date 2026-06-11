/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <functional>
#include <memory>

namespace deskflow::coordination {

//! Platform "genuine local hardware input" signal.
/*!
Fires the callback once per hardware-originated input event (mouse motion,
buttons, keys), and never for synthesized/injected events -- this is what
lets a client machine claim primary the instant its own mouse moves while
ignoring the motion the KVM server is injecting into it.

Implementations (see behavior-spec.md §3.1):
 - macOS: listen-only CGEventTap; genuine iff the event source unix PID
   is 0. Requires the same Input Monitoring grant deskflow already holds.
 - Windows: Raw Input sink; genuine iff RAWINPUTHEADER.hDevice != NULL.
 - others: unsupported stub (never fires; auto mode then only follows
   claims and manual promotes).
*/
class ILocalInputMonitor
{
public:
  using Callback = std::function<void()>;

  virtual ~ILocalInputMonitor() = default;

  //! Start delivering callbacks (from the monitor's own thread).
  virtual bool start(Callback onGenuineInput) = 0;
  virtual void stop() = 0;
};

//! Create the monitor for the current platform.
std::unique_ptr<ILocalInputMonitor> createLocalInputMonitor();

} // namespace deskflow::coordination
