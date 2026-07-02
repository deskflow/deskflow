/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/FleetStateMerge.h"

namespace deskflow::coordination {

namespace {

bool peersEqual(const std::vector<FleetPeer> &a, const std::vector<FleetPeer> &b)
{
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].name != b[i].name || a[i].ip != b[i].ip || a[i].lan != b[i].lan) {
      return false;
    }
  }
  return true;
}

bool linksEqual(const std::vector<FleetLink> &a, const std::vector<FleetLink> &b)
{
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].fromScreen != b[i].fromScreen || a[i].toScreen != b[i].toScreen || a[i].direction != b[i].direction) {
      return false;
    }
  }
  return true;
}

bool screensEqual(const std::vector<FleetScreen> &a, const std::vector<FleetScreen> &b)
{
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].name != b[i].name) {
      return false;
    }
  }
  return true;
}

} // namespace

bool operator==(const FleetState &a, const FleetState &b)
{
  return a.server == b.server && a.cursorHost == b.cursorHost && a.cursorScreen == b.cursorScreen && a.seq == b.seq &&
         peersEqual(a.peers, b.peers) && linksEqual(a.links, b.links) && screensEqual(a.screens, b.screens);
}

bool operator!=(const FleetState &a, const FleetState &b)
{
  return !(a == b);
}

FleetMergeResult applyServerFragment(FleetState &state, const FleetFragment &fragment)
{
  FleetMergeResult result;
  if (fragment.server.empty()) {
    return result;
  }
  if (fragment.seq < state.seq) {
    return result;
  }

  const bool hadTopology = !state.links.empty();
  const FleetState before = state;

  state.server = fragment.server;
  state.cursorHost = fragment.cursorHost;
  state.cursorScreen = fragment.cursorScreen;
  state.peers = fragment.peers;
  state.links = fragment.links;
  state.screens = fragment.screens;
  state.seq = fragment.seq;

  result.changed = state != before;
  result.topologyBecameReady = !hadTopology && !state.links.empty();
  return result;
}

} // namespace deskflow::coordination
