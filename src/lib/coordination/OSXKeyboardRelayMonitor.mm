/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/KeyboardRelayMonitor.h"

#include "coordination/KeyboardRelayMap.h"

#include "base/Log.h"

#include <ApplicationServices/ApplicationServices.h>

#include <atomic>
#include <cctype>
#include <thread>

namespace deskflow::coordination {

namespace {

bool hostsEqual(const std::string &a, const std::string &b)
{
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
      return false;
    }
  }
  return true;
}

class OSXKeyboardRelayMonitor : public IKeyboardRelayMonitor
{
public:
  ~OSXKeyboardRelayMonitor() override
  {
    stop();
  }

  bool start(const std::string &selfName, CursorHostQuery cursorHost, KeyForwardSend send) override
  {
    if (m_thread.joinable()) {
      return true;
    }
    m_selfHost = selfName;
    m_cursorHost = std::move(cursorHost);
    m_send = std::move(send);
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
    auto *self = static_cast<OSXKeyboardRelayMonitor *>(refcon);
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
      if (self->m_tap != nullptr) {
        CGEventTapEnable(self->m_tap, true);
      }
      return event;
    }

    if (type != kCGEventKeyDown && type != kCGEventKeyUp) {
      return event;
    }

    const auto sourcePid = CGEventGetIntegerValueField(event, kCGEventSourceUnixProcessID);
    if (sourcePid != 0) {
      return event;
    }

    const std::string cursorHost = self->m_cursorHost ? self->m_cursorHost() : std::string();
    const std::string selfHost = self->m_selfHost;
    if (cursorHost.empty() || hostsEqual(cursorHost, selfHost)) {
      return event;
    }

    Message::KeyPhase phase = Message::KeyPhase::Down;
    KeyID id = 0;
    KeyModifierMask mask = 0;
    KeyButton button = 0;
    if (!mapRelayKeyFromCgEvent(event, phase, id, mask, button)) {
      return event;
    }
    if (self->m_send) {
      self->m_send(phase, id, mask, button, {});
    }
    return nullptr;
  }

  void runLoop()
  {
    const CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);

    m_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, mask, tapCallback, this);
    if (m_tap == nullptr) {
      LOG_WARN("coordination: keyboard relay tap unavailable (input monitoring permission?)");
      return;
    }

    m_runLoop = CFRunLoopGetCurrent();
    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_tap, 0);
    CFRunLoopAddSource(m_runLoop, source, kCFRunLoopCommonModes);
    CGEventTapEnable(m_tap, true);
    LOG_DEBUG("coordination: keyboard relay monitor started");

    while (m_running) {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, true);
    }

    CGEventTapEnable(m_tap, false);
    CFRunLoopRemoveSource(m_runLoop, source, kCFRunLoopCommonModes);
    CFRelease(source);
    CFRelease(m_tap);
    m_tap = nullptr;
    LOG_DEBUG("coordination: keyboard relay monitor stopped");
  }

  CursorHostQuery m_cursorHost;
  KeyForwardSend m_send;
  std::string m_selfHost;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  CFMachPortRef m_tap = nullptr;
  CFRunLoopRef m_runLoop = nullptr;
};

} // namespace

std::unique_ptr<IKeyboardRelayMonitor> createKeyboardRelayMonitor()
{
  return std::make_unique<OSXKeyboardRelayMonitor>();
}

} // namespace deskflow::coordination
