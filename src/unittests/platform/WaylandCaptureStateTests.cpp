/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WaylandCaptureStateTests.h"

#include "platform/WaylandCaptureStateMachine.h"

// Coverage for recoverable Wayland capture invalidation cases.
void WaylandCaptureStateTests::eisDisconnectTransitionsToRebindRequired()
{
  deskflow::WaylandCaptureStateMachine machine;
  machine.onCaptureEstablished();
  machine.onEisDisconnected();

  QCOMPARE(machine.state(), deskflow::CaptureState::RebindRequired);
}

void WaylandCaptureStateTests::createSessionCanceledTransitionsToRebindRequired()
{
  deskflow::WaylandCaptureStateMachine machine;
  machine.onCreateSessionCanceled();

  QCOMPARE(machine.state(), deskflow::CaptureState::RebindRequired);
}

void WaylandCaptureStateTests::eisDisconnectExposesSessionLifecycleStatusMarkers()
{
  deskflow::WaylandCaptureStateMachine machine;
  machine.onCaptureEstablished();
  machine.onEisDisconnected();

  const auto status = QString::fromStdString(machine.statusSummary());
  QVERIFY(status.contains("wayland_capture_state=rebind_required"));
  QVERIFY(status.contains("wayland_failure_class=session_lifecycle"));
  QVERIFY(status.contains("wayland_failure_reason=eis_disconnected"));
}

void WaylandCaptureStateTests::createSessionCanceledExposesSessionLifecycleStatusMarkers()
{
  deskflow::WaylandCaptureStateMachine machine;
  machine.onCreateSessionCanceled();

  const auto status = QString::fromStdString(machine.statusSummary());
  QVERIFY(status.contains("wayland_capture_state=rebind_required"));
  QVERIFY(status.contains("wayland_failure_class=session_lifecycle"));
  QVERIFY(status.contains("wayland_failure_reason=create_session_canceled"));
}

QTEST_MAIN(WaylandCaptureStateTests)
