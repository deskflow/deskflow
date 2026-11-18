/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AppUtilUnixTests.h"

#include "base/EventQueue.h"
#include "deskflow/unix/AppUtilUnix.h"
#include "platform/OSXAutoTypes.h"

#include <Carbon/Carbon.h>

#include <atomic>
#include <thread>
#include <vector>

void AppUtilUnixTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Error);
}

void AppUtilUnixTests::cachedInputSourceDataIsThreadSafe()
{
  EventQueue events;
  AppUtilUnix util(&events);
  const auto expectedLanguage = util.getCurrentLanguageCode();
  const auto expectedLayouts = util.getKeyboardLayoutList();
  QVERIFY(!expectedLanguage.empty());
  QVERIFY(!expectedLayouts.empty());

  constexpr int workerCount = 4;
  constexpr int iterations = 1000;
  std::atomic<int> ready{0};
  std::atomic<int> failures{0};
  std::atomic<bool> start{false};
  std::vector<std::thread> workers;

  for (int worker = 0; worker < workerCount; ++worker) {
    workers.emplace_back([&] {
      ++ready;
      while (!start.load()) {
        std::this_thread::yield();
      }
      for (int i = 0; i < iterations; ++i) {
        if (util.getCurrentLanguageCode() != expectedLanguage || util.getKeyboardLayoutList() != expectedLayouts) {
          ++failures;
        }
      }
    });
  }

  while (ready != workerCount) {
    std::this_thread::yield();
  }
  start = true;

  for (int i = 0; i < iterations; ++i) {
    AutoTISInputSourceRef source(TISCopyCurrentKeyboardInputSource(), CFRelease);
    if (source) {
      TISGetInputSourceProperty(source.get(), kTISPropertyInputSourceLanguages);
    }
  }

  for (auto &worker : workers) {
    worker.join();
  }
  QCOMPARE(failures.load(), 0);
}

QTEST_MAIN(AppUtilUnixTests)
