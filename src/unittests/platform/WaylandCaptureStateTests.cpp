/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WaylandCaptureStateTests.h"

// Red-phase coverage for recoverable Wayland capture invalidation cases.
void WaylandCaptureStateTests::eisDisconnectTransitionsToRebindRequired()
{
  WaylandCaptureStateMachine machine;
  machine.onCaptureEstablished();
  machine.onEisDisconnected();

  QCOMPARE(machine.state(), CaptureState::RebindRequired);
}

void WaylandCaptureStateTests::createSessionCanceledTransitionsToRebindRequired()
{
  WaylandCaptureStateMachine machine;
  machine.onCreateSessionCanceled();

  QCOMPARE(machine.state(), CaptureState::RebindRequired);
}

QTEST_MAIN(WaylandCaptureStateTests)
