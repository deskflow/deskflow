/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "coordination/RelayKeyEvent.h"

//! Event data for CoordinationKeyForward (cursor host injects relayed keys).
class CoordinationKeyForwardInfo : public EventData
{
public:
  deskflow::coordination::RelayKeyEvent event;

  explicit CoordinationKeyForwardInfo(deskflow::coordination::RelayKeyEvent event) : event(std::move(event)) {}

  ~CoordinationKeyForwardInfo() override = default;
};
