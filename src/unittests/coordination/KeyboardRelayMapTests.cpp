/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardRelayMapTests.h"

#include "coordination/CoordinationProtocol.h"
#include "coordination/KeyboardRelayMap.h"
#include "deskflow/KeyTypes.h"

#include <atomic>
#include <chrono>
#include <thread>

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

using deskflow::coordination::Message;

void KeyboardRelayMapTests::mapRelayKeyFromCgEventOffMainThreadDoesNotCrash()
{
  CGEventRef event = CGEventCreateKeyboardEvent(nullptr, kVK_Space, true);
  QVERIFY(event != nullptr);

  std::atomic<bool> finished{false};
  std::atomic<bool> mapped{false};
  std::thread worker([&]() {
    Message::KeyPhase phase = Message::KeyPhase::Down;
    KeyID id = kKeyNone;
    KeyModifierMask mask = 0;
    KeyButton button = 0;
    mapped.store(
        deskflow::coordination::mapRelayKeyFromCgEvent(event, phase, id, mask, button), std::memory_order_relaxed
    );
    finished.store(true, std::memory_order_relaxed);
  });

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!finished.load(std::memory_order_relaxed) && std::chrono::steady_clock::now() < deadline) {
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
  }
  if (!finished.load(std::memory_order_relaxed)) {
    worker.detach();
    QFAIL("mapRelayKeyFromCgEvent did not complete within 5 seconds");
  }
  worker.join();
  CFRelease(event);

  QVERIFY(finished.load(std::memory_order_relaxed));
  QVERIFY(mapped.load(std::memory_order_relaxed));
}

QTEST_MAIN(KeyboardRelayMapTests)
