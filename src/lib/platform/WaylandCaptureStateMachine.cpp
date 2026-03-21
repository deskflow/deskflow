/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WaylandCaptureStateMachine.h"

namespace deskflow {

namespace {

const char *captureStateToStatusValue(CaptureState state)
{
  switch (state) {
  case CaptureState::Starting:
    return "starting";
  case CaptureState::Captured:
    return "captured";
  case CaptureState::Degraded:
    return "degraded";
  case CaptureState::RebindRequired:
    return "rebind_required";
  case CaptureState::Fatal:
    return "fatal";
  }

  return "starting";
}

} // namespace

CaptureState WaylandCaptureStateMachine::state() const
{
  return m_state;
}

std::string WaylandCaptureStateMachine::statusSummary() const
{
  std::string status = "wayland_capture_state=";
  status += captureStateToStatusValue(m_state);

  if (m_failureClass != nullptr) {
    status += " wayland_failure_class=";
    status += m_failureClass;
  }

  if (m_failureReason != nullptr) {
    status += " wayland_failure_reason=";
    status += m_failureReason;
  }

  return status;
}

void WaylandCaptureStateMachine::onCaptureEstablished()
{
  m_state = CaptureState::Captured;
  m_failureClass = nullptr;
  m_failureReason = nullptr;
}

void WaylandCaptureStateMachine::setSessionLifecycleFailure(const char *reason)
{
  m_state = CaptureState::RebindRequired;
  m_failureClass = "session_lifecycle";
  m_failureReason = reason;
}

void WaylandCaptureStateMachine::onEisDisconnected()
{
  setSessionLifecycleFailure("eis_disconnected");
}

void WaylandCaptureStateMachine::onCreateSessionCanceled()
{
  setSessionLifecycleFailure("create_session_canceled");
}

} // namespace deskflow
