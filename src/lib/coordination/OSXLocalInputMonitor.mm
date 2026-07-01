/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/LocalInputMonitor.h"

#include "base/Log.h"

#include <ApplicationServices/ApplicationServices.h>

#include <atomic>
#include <thread>

namespace deskflow::coordination {

namespace {

//! Listen-only CGEventTap on its own CFRunLoop thread.
/*!
Genuine hardware events carry an event-source unix PID of 0; anything
injected by deskflow (or any other process) carries the injector's PID
and is ignored. Replaces the external inputmon.swift helper.
*/
class OSXLocalInputMonitor : public ILocalInputMonitor
{
public:
  ~OSXLocalInputMonitor() override
  {
    stop();
  }

  bool start(Callback onGenuineInput) override
  {
    if (m_thread.joinable()) {
      return true;
    }
    m_callback = std::move(onGenuineInput);
    m_running = true;
    m_thread = std::thread([this] { runLoop(); });
    return true;
  }

  void stop() override
  {
    m_running = false;
    if (m_runLoop != nullptr) {
      CFRunLoopStop(m_runLoop);
    }
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_runLoop = nullptr;
  }

private:
  static CGEventRef tapCallback(CGEventTapProxy, CGEventType type, CGEventRef event, void *refcon)
  {
    auto *self = static_cast<OSXLocalInputMonitor *>(refcon);

    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
      // The OS disables stalled taps; listen-only taps still must re-arm.
      if (self->m_tap != nullptr) {
        CGEventTapEnable(self->m_tap, true);
      }
      return event;
    }

    const auto sourcePid = CGEventGetIntegerValueField(event, kCGEventSourceUnixProcessID);
    if (sourcePid == 0 && self->m_callback) {
      self->m_callback();
    }
    return event;
  }

  void runLoop()
  {
    const CGEventMask mask = CGEventMaskBit(kCGEventMouseMoved) | CGEventMaskBit(kCGEventLeftMouseDown) |
                             CGEventMaskBit(kCGEventRightMouseDown) | CGEventMaskBit(kCGEventOtherMouseDown) |
                             CGEventMaskBit(kCGEventScrollWheel);

    m_tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, mask, tapCallback, this
    );
    if (m_tap == nullptr) {
      LOG_WARN("coordination: could not create input event tap (missing input monitoring permission?)");
      return;
    }

    m_runLoop = CFRunLoopGetCurrent();
    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_tap, 0);
    CFRunLoopAddSource(m_runLoop, source, kCFRunLoopCommonModes);
    CGEventTapEnable(m_tap, true);
    LOG_DEBUG("coordination: local input monitor started");

    while (m_running) {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, true);
    }

    CGEventTapEnable(m_tap, false);
    CFRunLoopRemoveSource(m_runLoop, source, kCFRunLoopCommonModes);
    CFRelease(source);
    CFRelease(m_tap);
    m_tap = nullptr;
    LOG_DEBUG("coordination: local input monitor stopped");
  }

  Callback m_callback;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  CFMachPortRef m_tap = nullptr;
  CFRunLoopRef m_runLoop = nullptr;
};

} // namespace

std::unique_ptr<ILocalInputMonitor> createLocalInputMonitor()
{
  return std::make_unique<OSXLocalInputMonitor>();
}

} // namespace deskflow::coordination
