/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ElectionStateTests.h"

#include "coordination/ElectionState.h"

#include <QTest>

using deskflow::coordination::ElectionState;
using deskflow::coordination::ElectionTuning;
using deskflow::coordination::Role;
using ClaimAction = ElectionState::ClaimAction;

namespace {

//! Test fixture wrapping ElectionState with a controllable clock.
struct Fixture
{
  double now = 1000.0;
  ElectionState state;

  explicit Fixture(const std::string &name = "self", ElectionTuning tuning = {})
      : state(name, tuning, [this] { return now; })
  {
  }

  //! Feed \p count input ticks \p spacing seconds apart; returns whether
  //! any tick completed a promotion burst.
  bool feedInput(int count, double spacing)
  {
    bool promoted = false;
    for (int i = 0; i < count; ++i) {
      now += spacing;
      promoted = state.onLocalInput() || promoted;
    }
    return promoted;
  }
};

} // namespace

void ElectionStateTests::initialRoleIsInit()
{
  Fixture f;
  QCOMPARE(f.state.role(), Role::Init);
  QCOMPARE(f.state.seq(), 0);
}

void ElectionStateTests::inputBurstPromotes()
{
  Fixture f;
  QVERIFY(f.feedInput(4, 0.05));
}

void ElectionStateTests::slowInputNeverPromotes()
{
  Fixture f;
  // Each tick 0.5s apart: outside the 0.40s window, the burst never fills.
  QVERIFY(!f.feedInput(50, 0.5));
}

void ElectionStateTests::selfCooldownBlocksPromotion()
{
  Fixture f;
  f.state.becameClient("10.0.0.2");

  // Within the 2.5s self-cooldown nothing promotes, however fast.
  QVERIFY(!f.feedInput(20, 0.01));

  // After the cooldown, a burst promotes again.
  f.now += 3.0;
  QVERIFY(f.feedInput(4, 0.05));
}

void ElectionStateTests::serverNeverPromotesAgain()
{
  Fixture f;
  f.state.becameServer();
  f.now += 10.0; // far outside any cooldown
  QVERIFY(!f.feedInput(50, 0.01));
}

void ElectionStateTests::cursorHereRaisesThreshold()
{
  Fixture f;
  f.state.becameClient("10.0.0.2");
  f.now += 3.0;
  f.state.setCursorHere(true);

  // The normal threshold (4) must not promote while the cursor is here.
  QVERIFY(!f.feedInput(8, 0.05));
  // But a sustained 12-within-0.80s burst must.
  QVERIFY(f.feedInput(12, 0.05));
}

void ElectionStateTests::cursorToggleClearsBurst()
{
  Fixture f;
  // 3 fast ticks, then the cursor regime changes: stale ticks must not
  // count toward the next threshold.
  QVERIFY(!f.feedInput(3, 0.01));
  f.state.setCursorHere(true);
  QVERIFY(!f.feedInput(8, 0.01));
}

void ElectionStateTests::claimFromSelfIgnored()
{
  Fixture f("alpha");
  QCOMPARE(f.state.onClaim("alpha", "10.0.0.1", "alpha.local", 5), ClaimAction::Ignore);
}

void ElectionStateTests::claimDuringServerCooldownIgnored()
{
  Fixture f;
  f.state.becameServer();
  f.now += 1.0; // inside the 1.5s claim cooldown
  QCOMPARE(f.state.onClaim("beta", "10.0.0.2", "beta.local", 1), ClaimAction::Ignore);
}

void ElectionStateTests::claimAfterServerCooldownFollowed()
{
  Fixture f;
  f.state.becameServer();
  f.now += 2.0; // outside the cooldown
  QCOMPARE(f.state.onClaim("beta", "10.0.0.2", "beta.local", 1), ClaimAction::FollowSender);
}

void ElectionStateTests::clientSelfCooldownBlocksForeignClaim()
{
  // Regression: a client that just switched must NOT immediately follow a
  // claim from a different peer, or two machines ping-pong leadership and
  // churn epochs (which raced socket teardown into a use-after-free).
  Fixture f;
  f.state.becameClient("10.0.0.2"); // lastSwitchAt = now

  // Within the 2.5s self-cooldown a foreign peer's claim is ignored...
  f.now += 1.0;
  QCOMPARE(f.state.onClaim("other", "10.0.0.3", "other.local", 5), ClaimAction::Ignore);

  // ...but once the cooldown elapses the same claim is followed.
  f.now += 2.0; // 3.0s since the switch, past the cooldown
  QCOMPARE(f.state.onClaim("other", "10.0.0.3", "other.local", 6), ClaimAction::FollowSender);
}

void ElectionStateTests::sameHostHeartbeatIsNoOp()
{
  Fixture f;
  f.state.becameClient("10.0.0.2");
  f.now += 10.0;

  // Heartbeats from the host we already follow are strict no-ops,
  // whether they match by ip or by lan address.
  QCOMPARE(f.state.onClaim("beta", "10.0.0.2", "beta.local", 2), ClaimAction::Ignore);
  f.state.becameClient("beta.local");
  f.now += 10.0;
  QCOMPARE(f.state.onClaim("beta", "10.0.0.2", "beta.local", 3), ClaimAction::Ignore);

  // A different host's claim is followed.
  QCOMPARE(f.state.onClaim("gamma", "10.0.0.3", "gamma.local", 4), ClaimAction::FollowSender);
}

void ElectionStateTests::claimMergesSequenceNumbers()
{
  Fixture f;
  f.state.onClaim("beta", "10.0.0.2", "beta.local", 41);
  QCOMPARE(f.state.seq(), 41);
  // Lower sequence numbers never regress the gossip value.
  f.state.onClaim("gamma", "10.0.0.3", "gamma.local", 7);
  QCOMPARE(f.state.seq(), 41);
  QCOMPARE(f.state.nextClaimSeq(), 42);
}

void ElectionStateTests::transitionsResetBurstAndCursor()
{
  Fixture f;
  f.state.setCursorHere(true);
  f.feedInput(3, 0.01);

  f.state.becameServer();
  QCOMPARE(f.state.role(), Role::Server);
  QVERIFY(!f.state.cursorHere());

  f.state.becameClient("10.0.0.2");
  QCOMPARE(f.state.role(), Role::Client);
  QCOMPARE(f.state.serverAddress(), std::string("10.0.0.2"));
  QVERIFY(!f.state.cursorHere());
}

QTEST_MAIN(ElectionStateTests)
