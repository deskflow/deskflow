/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WaylandCaptureStateMachine.h"

namespace deskflow {

CaptureState WaylandCaptureStateMachine::state() const
{
  return m_state;
}

void WaylandCaptureStateMachine::onCaptureEstablished()
{
  m_state = CaptureState::Captured;
}

void WaylandCaptureStateMachine::onEisDisconnected()
{
  m_state = CaptureState::RebindRequired;
}

void WaylandCaptureStateMachine::onCreateSessionCanceled()
{
  m_state = CaptureState::RebindRequired;
}

} // namespace deskflow
