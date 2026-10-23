/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "deskflow/unix/AppUtilUnix.h"
#include "platform/IOSXKeyResource.h"
#include "platform/OSXKeyState.h"

#include <QApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

// Never creates an event tap, posts a key, selects a source or opens a clipboard.
class TISReadPathsTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void threadedReadPaths()
  {
    Arch arch;
    arch.init();
    Log log;
    log.setFilter(LogLevel::Level::Error);
    EventQueue events;
    AppUtilUnix util(&events);
    const auto expectedLanguage = util.getCurrentLanguageCode();
    const auto expectedLayouts = util.getKeyboardLayoutList();
    QVERIFY(!expectedLanguage.empty());
    QVERIFY(!expectedLayouts.empty());
    const auto expectedLegacyKey = IOSXKeyResource::getKeyID(128);
    deskflow::KeyMap referenceMap;
    OSXKeyState referenceState(&events, referenceMap, {"en"}, true);
    referenceState.updateKeyMap();
    const auto expectedGroup = referenceState.pollActiveGroup();
    auto referenceEvent = CGEventCreateKeyboardEvent(nullptr, 0, true);
    QVERIFY(referenceEvent);
    CGEventSetFlags(referenceEvent, 0);
    OSXKeyState::KeyIDs expectedIds;
    KeyModifierMask expectedModifiers = 0;
    const auto expectedButton = referenceState.mapKeyFromEvent(expectedIds, &expectedModifiers, referenceEvent);
    CFRelease(referenceEvent);
    QVERIFY(expectedButton != 0);
    QVERIFY(!expectedIds.empty());
    std::atomic<int> done{0}, passed{0}, mismatches{0};
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;
    for (int worker = 0; worker < 4; ++worker) {
      threads.emplace_back([&] {
        deskflow::KeyMap keyMap;
        OSXKeyState state(&events, keyMap, {"en"}, true);
        state.updateKeyMap();
        auto event = CGEventCreateKeyboardEvent(nullptr, 0, true);
        if (event)
          CGEventSetFlags(event, 0);
        for (int i = 0; i < 1000 && !stop; ++i) {
          if (state.pollActiveGroup() != expectedGroup)
            ++mismatches;
          if (util.getCurrentLanguageCode() != expectedLanguage)
            ++mismatches;
          if (util.getKeyboardLayoutList() != expectedLayouts)
            ++mismatches;
          if (IOSXKeyResource::getKeyID(128) != expectedLegacyKey)
            ++mismatches;
          if (event) {
            OSXKeyState::KeyIDs ids;
            KeyModifierMask modifiers = 0;
            const auto button = state.mapKeyFromEvent(ids, &modifiers, event);
            if (button != expectedButton || ids != expectedIds || modifiers != expectedModifiers)
              ++mismatches;
          } else
            ++mismatches;
          ++passed;
        }
        if (event)
          CFRelease(event);
        ++done;
      });
    }
    auto began = std::chrono::steady_clock::now();
    while (done != 4) {
      // Main-thread reads are independent, like AppKit's shortcut updater.
      auto source = TISCopyCurrentKeyboardLayoutInputSource();
      if (source)
        CFRelease(source);
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0001, true);
      if (std::chrono::steady_clock::now() - began > std::chrono::seconds(60)) {
        stop = true;
        // Keep servicing the main queue until workers finish their pending call.
      }
    }
    for (auto &thread : threads)
      thread.join();
    QVERIFY(!stop);
    QCOMPARE(passed.load(), 4000);
    QCOMPARE(mismatches.load(), 0);
  }
};

int main(int argc, char **argv)
{
  QTemporaryDir temporary;
  if (!temporary.isValid())
    return 1;
  const auto config = temporary.path() + "/config";
  const auto state = temporary.path() + "/state";
  if (!QDir().mkpath(config + "/Deskflow") || !QDir().mkpath(state))
    return 1;
  qputenv("XDG_CONFIG_HOME", config.toUtf8());
  qputenv("XDG_STATE_HOME", state.toUtf8());
  QApplication app(argc, argv);
  TISReadPathsTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "TISReadPathsTests.moc"
