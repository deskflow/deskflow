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

QTEST_MAIN(WaylandCaptureStateTests)
