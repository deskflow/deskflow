/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "VirtualHostTrackerTests.h"

#include "server/VirtualHostTracker.h"

#include <QTest>

#include <string>
#include <vector>

namespace {

struct SentLine
{
  void *client = nullptr;
  std::string line;
};

} // namespace

void VirtualHostTrackerTests::connectsOnRemoteFocus()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  std::vector<SentLine> sent;
  const auto send = [&](void *client, const std::string &line) {
    sent.push_back({client, line});
  };

  tracker.setConnectLine(R"({"type":"connect","device":"mouse"})");
  tracker.onFocusChange(
      reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary),
      [&](BaseClientProxy *client, const std::string &line) { send(client, line); }
  );

  QCOMPARE(sent.size(), static_cast<size_t>(1));
  QCOMPARE(sent[0].client, &remote);
  QCOMPARE(sent[0].line, R"({"type":"connect","device":"mouse"})");
  QCOMPARE(tracker.host(), reinterpret_cast<BaseClientProxy *>(&remote));
}

void VirtualHostTrackerTests::movesVirtualHostWhenFocusChanges()
{
  VirtualHostTracker tracker;
  char primary{};
  char remoteA{};
  char remoteB{};

  std::vector<SentLine> sent;
  const auto send = [&](BaseClientProxy *client, const std::string &line) {
    sent.push_back({client, line});
  };

  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remoteA), reinterpret_cast<BaseClientProxy *>(&primary), send);
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remoteB), reinterpret_cast<BaseClientProxy *>(&primary), send);

  QCOMPARE(sent.size(), static_cast<size_t>(3));
  QCOMPARE(sent[1].line, VirtualHostTracker::kDefaultDisconnect);
  QCOMPARE(sent[1].client, &remoteA);
  QCOMPARE(sent[2].client, &remoteB);
  QCOMPARE(tracker.host(), reinterpret_cast<BaseClientProxy *>(&remoteB));
}

void VirtualHostTrackerTests::skipsConnectOnPrimaryFocus()
{
  VirtualHostTracker tracker;
  char primary{};

  std::vector<SentLine> sent;
  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(
      reinterpret_cast<BaseClientProxy *>(&primary), reinterpret_cast<BaseClientProxy *>(&primary),
      [&](BaseClientProxy *client, const std::string &line) { sent.push_back({client, line}); }
  );

  QVERIFY(sent.empty());
  QCOMPARE(tracker.host(), nullptr);
}

void VirtualHostTrackerTests::detachClearsHost()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  const auto send = [](BaseClientProxy *, const std::string &) {};

  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary), send);
  tracker.clearHostIf(reinterpret_cast<BaseClientProxy *>(&remote));
  QCOMPARE(tracker.host(), nullptr);
}

QTEST_MAIN(VirtualHostTrackerTests)
