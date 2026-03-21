/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>

namespace deskflow {

enum class CaptureState : std::uint8_t
{
  Starting,
  Captured,
  Degraded,
  RebindRequired,
  Fatal
};

class WaylandCaptureStateMachine
{
public:
  WaylandCaptureStateMachine() = default;

  CaptureState state() const;

  void onCaptureEstablished();
  void onEisDisconnected();
  void onCreateSessionCanceled();

private:
  CaptureState m_state = CaptureState::Starting;
};

} // namespace deskflow
