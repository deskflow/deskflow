/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "VirtualHostTrackerTests.h"

#include "server/VirtualHostTracker.h"

#include <QTest>

#include <functional>
#include <string>
#include <vector>

namespace {

struct SentLine
{
  void *client = nullptr;
  std::string line;
};

using SendFn = std::function<void(BaseClientProxy *, const std::string &)>;

SendFn captureSend(std::vector<SentLine> &sent)
{
  return [&sent](BaseClientProxy *client, const std::string &line) {
    sent.push_back({client, line});
  };
}

} // namespace

void VirtualHostTrackerTests::connectsOnRemoteFocus()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  std::vector<SentLine> sent;
  tracker.setConnectLine(R"({"type":"connect","device":"mouse"})");
  tracker.onFocusChange(
      reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent)
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
  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remoteA), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent));
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remoteB), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent));

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
      reinterpret_cast<BaseClientProxy *>(&primary), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent)
  );

  QVERIFY(sent.empty());
  QCOMPARE(tracker.host(), nullptr);
}

void VirtualHostTrackerTests::clearHostIfClearsMatchingHost()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  std::vector<SentLine> sent;
  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent));
  tracker.clearHostIf(reinterpret_cast<BaseClientProxy *>(&remote));
  QCOMPARE(tracker.host(), nullptr);
}

void VirtualHostTrackerTests::detachSendsDisconnectAndClearsHost()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  std::vector<SentLine> sent;
  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent));
  tracker.detach(captureSend(sent), R"({"type":"custom-disconnect"})");

  QCOMPARE(sent.size(), static_cast<size_t>(2));
  QCOMPARE(sent[1].line, R"({"type":"custom-disconnect"})");
  QCOMPARE(sent[1].client, &remote);
  QCOMPARE(tracker.host(), nullptr);
}

void VirtualHostTrackerTests::hostsActiveClientMatchesRelayTarget()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  std::vector<SentLine> sent;
  tracker.setConnectLine(R"({"type":"connect"})");
  tracker.onFocusChange(reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent));

  QVERIFY(tracker.hostsActiveClient(reinterpret_cast<BaseClientProxy *>(&remote)));
  QVERIFY(!tracker.hostsActiveClient(reinterpret_cast<BaseClientProxy *>(&primary)));
}

void VirtualHostTrackerTests::connectPayloadOverrideBypassesCachedLine()
{
  VirtualHostTracker tracker;
  char primary{};
  char remote{};

  std::vector<SentLine> sent;
  tracker.setConnectLine(R"({"type":"connect","cached":true})");
  tracker.onFocusChange(
      reinterpret_cast<BaseClientProxy *>(&remote), reinterpret_cast<BaseClientProxy *>(&primary), captureSend(sent),
      R"({"type":"connect","override":true})"
  );

  QCOMPARE(sent.size(), static_cast<size_t>(1));
  QCOMPARE(sent[0].line, R"({"type":"connect","override":true})");
}

QTEST_MAIN(VirtualHostTrackerTests)
