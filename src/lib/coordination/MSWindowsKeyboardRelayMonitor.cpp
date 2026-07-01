/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/KeyboardRelayMonitor.h"

#include "coordination/KeyboardRelayMap.h"

#include "base/Log.h"

#include <windows.h>

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

class MSWindowsKeyboardRelayMonitor : public IKeyboardRelayMonitor
{
public:
  ~MSWindowsKeyboardRelayMonitor() override
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

    const std::string cursorHost = self->m_cursorHost ? self->m_cursorHost() : std::string();
    if (cursorHost.empty() || hostsEqual(cursorHost, self->m_selfHost)) {
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

  CursorHostQuery m_cursorHost;
  KeyForwardSend m_send;
  std::string m_selfHost;
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
