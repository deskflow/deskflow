/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileDragHelper.h"

#include "deskflow/FileDragCodec.h"

#include <cstdarg>
#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>

#include <objbase.h>
#include <ole2.h>
#include <shellapi.h>

namespace deskflow::filedrag::helper {

namespace {

constexpr UINT kMsgTask = WM_APP + 1;

Options g_options;
HINSTANCE g_instance = nullptr;
HWND g_control = nullptr;
HANDLE g_in = nullptr;
pipe::FrameWriter g_out;
std::mutex g_tasksMutex;
std::deque<std::function<void()>> g_tasks;

LRESULT CALLBACK controlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == kMsgTask) {
    while (true) {
      std::function<void()> task;
      {
        std::lock_guard lock(g_tasksMutex);
        if (g_tasks.empty()) {
          break;
        }
        task = std::move(g_tasks.front());
        g_tasks.pop_front();
      }
      task();
    }
    return 0;
  }
  if (msg == WM_TIMER && wParam == 7) {
    destRetryRemoval();
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void quit()
{
  sourceShutdown();
  destShutdown();
  PostQuitMessage(0);
}

bool readPoint(ByteReader &reader, int32_t &x, int32_t &y)
{
  uint32_t ux = 0;
  uint32_t uy = 0;
  if (!reader.u32(ux) || !reader.u32(uy)) {
    return false;
  }
  x = static_cast<int32_t>(ux);
  y = static_cast<int32_t>(uy);
  return true;
}

void sendSyncAck(uint32_t token)
{
  ByteWriter writer;
  writer.u32(token);
  sendFrame(pipe::Frame::SyncAck, writer.str());
}

void handleFrame(pipe::Frame frame, const std::string &payload)
{
  using pipe::Frame;
  ByteReader reader(payload);
  switch (frame) {
  case Frame::WatchEdges: {
    uint32_t sides = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t w = 0;
    int32_t h = 0;
    if (reader.u32(sides) && readPoint(reader, x, y) && readPoint(reader, w, h)) {
      RECT screen = {x, y, x + w, y + h};
      postToUi([sides, screen] { sourceWatchEdges(sides, screen); });
    }
    break;
  }

  case Frame::HandOff: {
    uint32_t token = 0;
    uint32_t dragId = 0;
    int32_t x = 0;
    int32_t y = 0;
    if (reader.u32(token) && reader.u32(dragId) && readPoint(reader, x, y)) {
      postToUi([token, dragId, x, y] {
        sourceHandOff(dragId, POINT{x, y});
        sendSyncAck(token);
      });
    }
    break;
  }

  case Frame::TakeBack: {
    uint32_t token = 0;
    if (reader.u32(token)) {
      postToUi([token] {
        sourceTakeBack();
        sendSyncAck(token);
      });
    }
    break;
  }

  case Frame::Entered:
    postToUi([] { sourceEntered(); });
    break;

  case Frame::Peer: {
    Message message;
    std::string error;
    if (!decodeMessage(payload, message, &error)) {
      log(LogLevel::Warning, "bad file drag message from the core: %s", error.c_str());
      break;
    }
    // both roles run on either computer: drags leave from here (source) and arrive here
    // (destination). Drag ids are unique across both directions (the server numbers them all).
    switch (message.type) {
      using enum Message::Type;
    case Ack:
    case Dropped:
    case Finished:
      sourcePeer(message);
      break;
    case Cancel:
      // each role only acts on its own drags
      sourcePeer(message);
      destPeer(message);
      break;
    default:
      destPeer(message);
      break;
    }
    break;
  }

  case Frame::TestSourceEnter: {
    if (!g_options.test) {
      break;
    }
    uint32_t count = 0;
    std::vector<std::wstring> paths;
    if (reader.u32(count)) {
      for (uint32_t i = 0; i < count; ++i) {
        std::string path;
        if (!reader.bytes16(path)) {
          break;
        }
        paths.push_back(pipe::widen(path));
      }
    }
    postToUi([paths] { sourceTestEnter(paths); });
    break;
  }

  case Frame::TestSourceLeave:
    if (g_options.test) {
      postToUi([] { sourceTestLeave(); });
    }
    break;

  case Frame::TestDropTarget: {
    std::string folder;
    if (g_options.test && reader.bytes16(folder)) {
      auto wide = pipe::widen(folder);
      postToUi([wide] { destTestDropTarget(wide); });
    }
    break;
  }

  case Frame::TestRelease: {
    uint8_t escape = 0;
    if (g_options.test && reader.u8(escape)) {
      destTestRelease(escape != 0);
    }
    break;
  }

  default:
    log(LogLevel::Debug, "ignoring frame %s", pipe::frameName(frame));
    break;
  }
}

void readerThread()
{
  // the staged-file streams live in the multi-threaded apartment, which this thread keeps alive
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  pipe::Frame frame;
  std::string payload;
  while (pipe::readFrame(g_in, frame, payload)) {
    if (frame == pipe::Frame::Shutdown) {
      break;
    }
    handleFrame(frame, payload);
  }
  postToUi([] { quit(); });
  // keep the apartment until the process ends
}

uintptr_t parseHandle(const wchar_t *text)
{
  return static_cast<uintptr_t>(_wcstoui64(text, nullptr, 10));
}

} // namespace

const Options &options()
{
  return g_options;
}

HINSTANCE instance()
{
  return g_instance;
}

HWND controlWindow()
{
  return g_control;
}

void postToUi(std::function<void()> task)
{
  {
    std::lock_guard lock(g_tasksMutex);
    g_tasks.push_back(std::move(task));
  }
  PostMessageW(g_control, kMsgTask, 0, 0);
}

void sendFrame(pipe::Frame frame, std::string_view payload)
{
  g_out.write(frame, payload);
}

void sendPeer(const Message &message)
{
  sendFrame(pipe::Frame::Peer, encodeMessage(message));
}

void testEvent(const std::string &text)
{
  if (g_options.test) {
    sendFrame(pipe::Frame::TestEvent, text);
  }
}

void log(LogLevel level, const char *format, ...)
{
  char buffer[2048];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  OutputDebugStringA("deskflow-filedrag: ");
  OutputDebugStringA(buffer);
  OutputDebugStringA("\n");
  ByteWriter writer;
  writer.u8(static_cast<uint8_t>(level));
  writer.bytes32(buffer);
  sendFrame(pipe::Frame::Log, writer.str());
}

HWND createOverlayWindow(const wchar_t *className, WNDPROC proc, void *self)
{
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  if (!GetClassInfoExW(g_instance, className, &wc)) {
    wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = proc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);
  }
  HWND hwnd = CreateWindowExW(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED, className, L"", WS_POPUP, 0, 0, 1, 1,
      nullptr, nullptr, g_instance, self
  );
  if (hwnd != nullptr) {
    // practically invisible, but still hit by the mouse and by drag and drop
    SetLayeredWindowAttributes(hwnd, 0, 1, LWA_ALPHA);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  return hwnd;
}

} // namespace deskflow::filedrag::helper

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
  using namespace deskflow::filedrag;
  using namespace deskflow::filedrag::helper;

  g_instance = instance;
  int argc = 0;
  wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  uintptr_t in = 0;
  uintptr_t out = 0;
  std::wstring dpi = L"permonitor";
  for (int i = 1; argv != nullptr && i < argc; ++i) {
    const std::wstring arg = argv[i];
    if (arg == L"--pipe-in" && i + 1 < argc) {
      in = parseHandle(argv[++i]);
    } else if (arg == L"--pipe-out" && i + 1 < argc) {
      out = parseHandle(argv[++i]);
    } else if (arg == L"--primary") {
      g_options.primary = true;
    } else if (arg == L"--secondary") {
      g_options.primary = false;
    } else if (arg == L"--test") {
      g_options.test = true;
    } else if (arg == L"--dpi" && i + 1 < argc) {
      dpi = argv[++i];
    } else if (arg == L"--staging-root" && i + 1 < argc) {
      g_options.stagingRoot = argv[++i];
    }
  }
  if (argv != nullptr) {
    LocalFree(argv);
  }
  if (in == 0 || out == 0) {
    MessageBoxW(
        nullptr, L"This program is part of Deskflow and is started by it.", L"Deskflow file drag",
        MB_OK | MB_ICONINFORMATION
    );
    return 1;
  }
  g_in = reinterpret_cast<HANDLE>(in);
  g_out.setPipe(reinterpret_cast<HANDLE>(out));

  // the same coordinates as deskflow-core: window positions and the cursor must match
  if (dpi == L"unaware") {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
  } else if (dpi == L"system") {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
  } else {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (FAILED(OleInitialize(nullptr))) {
    return 2;
  }

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = controlProc;
  wc.hInstance = instance;
  wc.lpszClassName = L"DeskflowFileDragControl";
  RegisterClassExW(&wc);
  g_control = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);

  destStart();

  ByteWriter hello;
  // both roles, and the client to server messages
  hello.u32(kCapSource | kCapDestination | kCapReverse);
  hello.u32(GetCurrentProcessId());
  sendFrame(pipe::Frame::Hello, hello.str());
  log(LogLevel::Debug, "file drag helper ready (%s%s)", g_options.primary ? "primary" : "secondary",
      g_options.test ? ", test" : "");

  std::thread reader(readerThread);
  reader.detach();

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  OleUninitialize();
  // the reader may still be blocked on the pipe; leave without waiting for it
  ExitProcess(0);
}
