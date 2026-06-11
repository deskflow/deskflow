/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/LocalInputMonitor.h"

#include "base/Log.h"

namespace deskflow::coordination {

namespace {

//! No genuine-input detection on this platform yet.
/*!
Auto mode still works: this node follows inbound claims and manual
promotes (kvmctl primary), it just never self-promotes from local input.
*/
class StubLocalInputMonitor : public ILocalInputMonitor
{
public:
  bool start(Callback) override
  {
    LOG_WARN("coordination: local input detection not supported on this platform");
    return false;
  }

  void stop() override
  {
    // do nothing
  }
};

} // namespace

std::unique_ptr<ILocalInputMonitor> createLocalInputMonitor()
{
  return std::make_unique<StubLocalInputMonitor>();
}

} // namespace deskflow::coordination
