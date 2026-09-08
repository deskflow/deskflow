/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"

namespace deskflow {

struct SharedInputLockEvent : EventData
{
  explicit SharedInputLockEvent(uint64_t value, bool released = true) : generation(value), success(released)
  {
  }

  uint64_t generation;
  bool success;
};

struct SharedInputLockRequest : EventData
{
  explicit SharedInputLockRequest(unsigned seconds) : autoReleaseSeconds(seconds)
  {
  }

  unsigned autoReleaseSeconds;
};

} // namespace deskflow
