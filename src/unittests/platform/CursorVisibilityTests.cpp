/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CursorVisibilityTests.h"

#include "platform/CursorVisibility.h"

#include <QTest>

#include <atomic>
#include <barrier>
#include <thread>
#include <vector>

namespace {

struct CountingCursorBackend
{
  bool apply(bool hidden)
  {
    if (hidden) {
      ++hideAttempts;
    } else {
      ++showAttempts;
    }
    if (failNext) {
      failNext = false;
      return false;
    }
    hideCount += hidden ? 1 : -1;
    invalidCount = invalidCount || hideCount < 0 || hideCount > 1;
    return true;
  }

  int hideCount = 0;
  int hideAttempts = 0;
  int showAttempts = 0;
  bool failNext = false;
  bool invalidCount = false;
};

} // namespace

void CursorVisibilityTests::setHidden_localRestoreAndRemoteTransitions_balancesNativeCount()
{
  deskflow::CursorVisibility visibility;
  CountingCursorBackend backend;
  const auto setHidden = [&](bool hidden) {
    return visibility.setHidden(hidden, [&] { return backend.apply(hidden); });
  };

  // Stop before the first connection must not issue an unmatched native show.
  QVERIFY(setHidden(false));
  QCOMPARE(backend.showAttempts, 0);

  // Enable hides the client; the first local movement restores it once.
  QVERIFY(setHidden(true));
  QCOMPARE(backend.hideCount, 1);
  QVERIFY(setHidden(false));
  QVERIFY(setHidden(false));
  QCOMPARE(backend.showAttempts, 1);

  // Remote entry after local use must not consume another native hide count.
  QVERIFY(setHidden(false));
  QCOMPARE(backend.showAttempts, 1);
  QVERIFY(setHidden(true));
  QVERIFY(setHidden(true));
  QCOMPARE(backend.hideCount, 1);
  QCOMPARE(backend.hideAttempts, 2);

  // Local restore, disconnect, and reconnect start another balanced cycle.
  QVERIFY(setHidden(false));
  QVERIFY(setHidden(false));
  QVERIFY(setHidden(true));
  QVERIFY(setHidden(false));
  QVERIFY(setHidden(false));

  QCOMPARE(backend.hideAttempts, 3);
  QCOMPARE(backend.showAttempts, 3);
  QCOMPARE(backend.hideCount, 0);
  QVERIFY(!backend.invalidCount);
}

void CursorVisibilityTests::setHidden_failedTransitions_canBeRetried()
{
  deskflow::CursorVisibility visibility;
  CountingCursorBackend backend;
  const auto setHidden = [&](bool hidden) {
    return visibility.setHidden(hidden, [&] { return backend.apply(hidden); });
  };

  backend.failNext = true;
  QVERIFY(!setHidden(true));
  QCOMPARE(backend.hideCount, 0);
  QVERIFY(setHidden(false));
  QCOMPARE(backend.showAttempts, 0);

  QVERIFY(setHidden(true));
  QCOMPARE(backend.hideAttempts, 2);
  QCOMPARE(backend.hideCount, 1);
  QVERIFY(setHidden(true));
  QCOMPARE(backend.hideAttempts, 2);

  backend.failNext = true;
  QVERIFY(!setHidden(false));
  QCOMPARE(backend.hideCount, 1);
  QVERIFY(setHidden(true));
  QCOMPARE(backend.hideAttempts, 2);

  // The next local movement retries a failed show rather than accepting it as visible.
  QVERIFY(setHidden(false));
  QCOMPARE(backend.showAttempts, 2);
  QCOMPARE(backend.hideCount, 0);
  QVERIFY(setHidden(false));
  QCOMPARE(backend.showAttempts, 2);
  QVERIFY(!backend.invalidCount);
}

void CursorVisibilityTests::setHidden_concurrentTransitions_serializeNativeCalls()
{
  deskflow::CursorVisibility visibility;
  std::atomic<int> activeCalls = 0;
  std::atomic<int> nativeHideCount = 0;
  std::atomic<int> nativeCalls = 0;
  std::atomic<bool> overlappingCalls = false;
  std::atomic<bool> invalidCount = false;

  const auto apply = [&](bool hidden) {
    if (activeCalls.fetch_add(1) != 0) {
      overlappingCalls.store(true);
    }
    // Yield inside the native operation to expose an unlocked check/apply/commit sequence.
    std::this_thread::yield();
    const int previousCount = nativeHideCount.fetch_add(hidden ? 1 : -1);
    if (previousCount != (hidden ? 0 : 1)) {
      invalidCount.store(true);
    }
    ++nativeCalls;
    std::this_thread::yield();
    --activeCalls;
    return true;
  };

  constexpr int workerCount = 8;
  constexpr int transitionsPerWorker = 1000;
  std::barrier start(workerCount);
  std::vector<std::thread> workers;
  workers.reserve(workerCount);
  for (int worker = 0; worker < workerCount; ++worker) {
    workers.emplace_back([&] {
      start.arrive_and_wait();
      for (int transition = 0; transition < transitionsPerWorker; ++transition) {
        visibility.setHidden(true, [&] { return apply(true); });
        visibility.setHidden(false, [&] { return apply(false); });
      }
    });
  }
  for (auto &worker : workers) {
    worker.join();
  }

  QVERIFY(!overlappingCalls.load());
  QVERIFY(!invalidCount.load());
  QCOMPARE(activeCalls.load(), 0);
  QCOMPARE(nativeHideCount.load(), 0);
  QVERIFY(nativeCalls.load() > 0);

  // Verify that the committed state agrees with the native count after contention.
  const int completedCalls = nativeCalls.load();
  QVERIFY(visibility.setHidden(false, [&] { return apply(false); }));
  QCOMPARE(nativeCalls.load(), completedCalls);
  QVERIFY(visibility.setHidden(true, [&] { return apply(true); }));
  QCOMPARE(nativeHideCount.load(), 1);
  QVERIFY(visibility.setHidden(false, [&] { return apply(false); }));
  QCOMPARE(nativeHideCount.load(), 0);
  QCOMPARE(nativeCalls.load(), completedCalls + 2);
}

QTEST_APPLESS_MAIN(CursorVisibilityTests)
