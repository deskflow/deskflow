/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ResumePolicyTests.h"

using deskflow::decideResumeAction;
using deskflow::ResumeAction;

// Normal suspend->resume: the client disconnected on suspend, so it reconnects
// iff it had a connection to restore.
void ResumePolicyTests::suspendResumeReconnectsWhenPreviouslyConnected()
{
  QVERIFY(decideResumeAction(/*wasSuspended*/ true, /*wanted*/ true, /*connected*/ false) == ResumeAction::Reconnect);
  QVERIFY(decideResumeAction(true, true, true) == ResumeAction::Reconnect);
}

void ResumePolicyTests::suspendResumeDoesNothingWhenNotPreviouslyConnected()
{
  QVERIFY(decideResumeAction(true, false, false) == ResumeAction::None);
  QVERIFY(decideResumeAction(true, false, true) == ResumeAction::None);
}

// The fix: a resume with no recorded suspend (e.g. Windows 10/11, where the
// suspend signal is dropped) must not be a silent no-op while the stale
// connection still appears live -- it must be dropped so auto-reconnect runs.
void ResumePolicyTests::missedSuspendWhileConnectedDropsStaleConnection()
{
  QVERIFY(decideResumeAction(/*wasSuspended*/ false, /*wanted*/ false, /*connected*/ true) == ResumeAction::DropStaleConnection);
  QVERIFY(decideResumeAction(false, true, true) == ResumeAction::DropStaleConnection);
}

// A resume with no suspend while already disconnected: auto-reconnect is already
// in flight, so do nothing.
void ResumePolicyTests::missedSuspendWhileDisconnectedDoesNothing()
{
  QVERIFY(decideResumeAction(false, false, false) == ResumeAction::None);
  QVERIFY(decideResumeAction(false, true, false) == ResumeAction::None);
}

QTEST_MAIN(ResumePolicyTests)
