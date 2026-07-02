/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FleetStateMergeTests.h"

#include "coordination/FleetStateMerge.h"

#include <QTest>

using deskflow::coordination::FleetFragment;
using deskflow::coordination::FleetLink;
using deskflow::coordination::FleetPeer;
using deskflow::coordination::FleetScreen;
using deskflow::coordination::FleetState;
using deskflow::coordination::applyServerFragment;

void FleetStateMergeTests::emptyFragmentIsNoop()
{
  FleetState state;
  state.server = "alpha";
  const auto result = applyServerFragment(state, FleetFragment{});
  QVERIFY(!result.changed);
  QCOMPARE(state.server, std::string("alpha"));
}

void FleetStateMergeTests::serverFragmentReplacesTopology()
{
  FleetState state;
  FleetFragment fragment;
  fragment.server = "server";
  fragment.seq = 2;
  fragment.cursorHost = "remote";
  fragment.cursorScreen = "remote";
  fragment.peers = {FleetPeer{"server", "10.0.0.1", "server.local"}, FleetPeer{"remote", "10.0.0.2", "remote.local"}};
  fragment.links = {FleetLink{"server", "remote", "right"}};
  fragment.screens = {FleetScreen{"server"}, FleetScreen{"remote"}};

  const auto result = applyServerFragment(state, fragment);

  QVERIFY(result.changed);
  QCOMPARE(state.server, std::string("server"));
  QCOMPARE(state.links.size(), static_cast<size_t>(1));
  QCOMPARE(state.peers.size(), static_cast<size_t>(2));
  QCOMPARE(state.screens.size(), static_cast<size_t>(2));
  QCOMPARE(state.cursorHost, std::string("remote"));
}

void FleetStateMergeTests::staleFragmentIsIgnored()
{
  FleetState state;
  state.server = "server";
  state.seq = 10;
  state.links = {FleetLink{"server", "remote", "right"}};

  FleetFragment stale;
  stale.server = "server";
  stale.seq = 5;
  stale.links = {FleetLink{"server", "other", "left"}};

  const auto result = applyServerFragment(state, stale);

  QVERIFY(!result.changed);
  QCOMPARE(state.links.front().toScreen, std::string("remote"));
  QCOMPARE(state.seq, static_cast<int64_t>(10));
}

void FleetStateMergeTests::equalSeqReplacesStaleLinks()
{
  FleetState state;
  state.server = "server";
  state.seq = 4;
  state.links = {FleetLink{"server", "remote", "right"}};
  state.screens = {FleetScreen{"server"}, FleetScreen{"remote"}};

  FleetFragment updated;
  updated.server = "server";
  updated.seq = 4;
  updated.links = {FleetLink{"server", "laptop", "left"}};
  updated.screens = {FleetScreen{"server"}, FleetScreen{"laptop"}};

  const auto result = applyServerFragment(state, updated);

  QVERIFY(result.changed);
  QCOMPARE(state.links.front().toScreen, std::string("laptop"));
  QCOMPARE(state.seq, static_cast<int64_t>(4));
}

void FleetStateMergeTests::cursorUpdatePreservesOrdering()
{
  FleetState state;
  FleetFragment first;
  first.server = "server";
  first.seq = 1;
  first.cursorHost = "a";
  first.cursorScreen = "a";
  applyServerFragment(state, first);

  FleetFragment second;
  second.server = "server";
  second.seq = 3;
  second.cursorHost = "b";
  second.cursorScreen = "b";
  second.links = first.links;
  second.peers = first.peers;
  second.screens = first.screens;

  const auto result = applyServerFragment(state, second);

  QVERIFY(result.changed);
  QCOMPARE(state.cursorHost, std::string("b"));
  QCOMPARE(state.seq, static_cast<int64_t>(3));
}

void FleetStateMergeTests::topologyBecameReadyFlag()
{
  FleetState state;
  FleetFragment fragment;
  fragment.server = "server";
  fragment.seq = 1;
  fragment.links = {FleetLink{"server", "remote", "right"}};

  const auto result = applyServerFragment(state, fragment);

  QVERIFY(result.topologyBecameReady);
}

void FleetStateMergeTests::topologyBecameReady_notSetWhenAlreadyReady()
{
  FleetState state;
  state.server = "server";
  state.seq = 2;
  state.links = {FleetLink{"server", "remote", "right"}};

  FleetFragment fragment;
  fragment.server = "server";
  fragment.seq = 3;
  fragment.cursorHost = "remote";
  fragment.links = state.links;

  const auto result = applyServerFragment(state, fragment);

  QVERIFY(result.changed);
  QVERIFY(!result.topologyBecameReady);
}

QTEST_MAIN(FleetStateMergeTests)
