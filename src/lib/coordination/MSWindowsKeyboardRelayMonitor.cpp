/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/KeyboardRelayDecision.h"
#include "coordination/KeyboardRelayMonitor.h"

#include "coordination/KeyboardRelayMap.h"

#include "base/Log.h"

#include <windows.h>

#include <atomic>
#include <thread>

namespace deskflow::coordination {

namespace {

class MSWindowsKeyboardRelayMonitor : public IKeyboardRelayMonitor
{
public:
  ~MSWindowsKeyboardRelayMonitor() override
  {
    stop();
  }

  bool start(CursorOnSelfQuery cursorOnSelf, KeyForwardSend send) override
  {
    if (m_thread.joinable()) {
      return true;
    }
    m_cursorOnSelf = std::move(cursorOnSelf);
    m_send = std::move(send);
    m_running = true;
    m_thread = std::thread([this] { runLoop(); });
    return true;
  }

  void stop() override
  {
    m_running = false;
    if (m_threadId != 0) {
      PostThreadMessageW(m_threadId, WM_QUIT, 0, 0);
    }
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_threadId = 0;
  }

private:
  static LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam)
  {
    if (code < 0) {
      return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    auto *self = g_relayInstance;
    if (self == nullptr) {
      return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    const auto *info = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
    const bool keyUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
    const bool isRepeat = !keyUp && (info->flags & LLKHF_UP) == 0 && (GetAsyncKeyState(info->vkCode) & 0x8000);

    const bool cursorOnSelf = self->m_cursorOnSelf ? self->m_cursorOnSelf() : true;
    if (passKeyToLocalOs(cursorOnSelf, self->m_cursorOnSelf != nullptr)) {
      return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    Message::KeyPhase phase = Message::KeyPhase::Down;
    KeyID id = 0;
    KeyModifierMask mask = 0;
    KeyButton button = 0;
    if (!mapRelayKeyFromHook(
            static_cast<int>(info->vkCode), static_cast<int>(info->scanCode),
            (info->flags & LLKHF_EXTENDED) != 0, keyUp, isRepeat, id, mask, button, phase
        )) {
      return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    if (self->m_send) {
      self->m_send(phase, id, mask, button, {});
    }
    return 1;
  }

  void runLoop()
  {
    m_threadId = GetCurrentThreadId();
    g_relayInstance = this;

    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, hookProc, GetModuleHandleW(nullptr), 0);
    if (m_hook == nullptr) {
      LOG_WARN("coordination: keyboard relay hook unavailable");
      g_relayInstance = nullptr;
      return;
    }
    LOG_DEBUG("coordination: keyboard relay monitor started");

    MSG message;
    while (m_running && GetMessageW(&message, nullptr, 0, 0) > 0) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }

    UnhookWindowsHookEx(m_hook);
    m_hook = nullptr;
    g_relayInstance = nullptr;
    LOG_DEBUG("coordination: keyboard relay monitor stopped");
  }

  CursorOnSelfQuery m_cursorOnSelf;
  KeyForwardSend m_send;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  DWORD m_threadId = 0;
  HHOOK m_hook = nullptr;
};

MSWindowsKeyboardRelayMonitor *g_relayInstance = nullptr;

} // namespace

std::unique_ptr<IKeyboardRelayMonitor> createKeyboardRelayMonitor()
{
  return std::make_unique<MSWindowsKeyboardRelayMonitor>();
}

} // namespace deskflow::coordination
