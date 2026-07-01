/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/KeyboardRelayMonitor.h"

namespace deskflow::coordination {

class StubKeyboardRelayMonitor : public IKeyboardRelayMonitor
{
public:
  bool start(RelayPassThroughQuery, KeyForwardSend) override
  {
    return true;
  }
  void stop() override
  {
  }
};

std::unique_ptr<IKeyboardRelayMonitor> createKeyboardRelayMonitor()
{
  return std::make_unique<StubKeyboardRelayMonitor>();
}

} // namespace deskflow::coordination
