/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/LocalInputMonitor.h"

#include "base/Log.h"

#include <windows.h>

#include <atomic>
#include <thread>

namespace deskflow::coordination {

namespace {

//! Raw Input sink on a message-only window thread.
/*!
Genuine hardware events arrive with a non-null RAWINPUTHEADER.hDevice;
SendInput-synthesized events (deskflow client injection) carry a null
device handle and are ignored. Replaces the external KvmSwitch.cs
Raw Input window.
*/
class MSWindowsLocalInputMonitor : public ILocalInputMonitor
{
public:
  ~MSWindowsLocalInputMonitor() override
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
    if (m_threadId != 0) {
      PostThreadMessageW(m_threadId, WM_QUIT, 0, 0);
    }
    if (m_thread.joinable()) {
      m_thread.join();
    }
    m_threadId = 0;
  }

private:
  static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    if (msg == WM_INPUT) {
      auto *self = reinterpret_cast<MSWindowsLocalInputMonitor *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
      if (self != nullptr) {
        self->handleRawInput(lParam);
      }
      return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }

  void handleRawInput(LPARAM lParam)
  {
    RAWINPUTHEADER header;
    UINT size = sizeof(header);
    const auto rc = GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam), RID_HEADER, &header, &size, sizeof(RAWINPUTHEADER)
    );
    if (rc == static_cast<UINT>(-1)) {
      return;
    }
    // Null device == synthesized (SendInput); only real hardware counts.
    if (header.hDevice != nullptr && m_callback) {
      m_callback();
    }
  }

  void runLoop()
  {
    m_threadId = GetCurrentThreadId();

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"DeskflowCoordinationInput";
    RegisterClassExW(&windowClass);

    HWND hwnd = CreateWindowExW(
        0, windowClass.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, windowClass.hInstance, nullptr
    );
    if (hwnd == nullptr) {
      LOG_WARN("coordination: could not create raw input window");
      return;
    }
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    RAWINPUTDEVICE devices[1]{};
    devices[0].usUsagePage = 0x01; // generic desktop
    devices[0].usUsage = 0x02;     // mouse
    devices[0].dwFlags = RIDEV_INPUTSINK;
    devices[0].hwndTarget = hwnd;
    if (!RegisterRawInputDevices(devices, 1, sizeof(RAWINPUTDEVICE))) {
      LOG_WARN("coordination: raw input registration failed");
      DestroyWindow(hwnd);
      return;
    }
    LOG_DEBUG("coordination: local input monitor started");

    MSG message;
    while (m_running && GetMessageW(&message, nullptr, 0, 0) > 0) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }

    DestroyWindow(hwnd);
    LOG_DEBUG("coordination: local input monitor stopped");
  }

  Callback m_callback;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  DWORD m_threadId = 0;
};

} // namespace

std::unique_ptr<ILocalInputMonitor> createLocalInputMonitor()
{
  return std::make_unique<MSWindowsLocalInputMonitor>();
}

} // namespace deskflow::coordination
