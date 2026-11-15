/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsFileDragBridge.h"

#include "arch/win32/ArchMiscWindows.h"
#include "base/Log.h"
#include "deskflow/FileDragCodec.h"
#include "platform/MSWindowsSession.h"

#include <filesystem>
#include <stdexcept>

using deskflow::filedrag::ByteReader;
using deskflow::filedrag::ByteWriter;
namespace pipe = deskflow::filedrag::pipe;

namespace {

bool isLocalSystem()
{
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }
  char buffer[256];
  DWORD size = 0;
  bool system = false;
  if (GetTokenInformation(token, TokenUser, buffer, sizeof(buffer), &size)) {
    const auto *user = reinterpret_cast<const TOKEN_USER *>(buffer);
    system = IsWellKnownSid(user->User.Sid, WinLocalSystemSid) != FALSE;
  }
  CloseHandle(token);
  return system;
}

//! The helper must see the same coordinates as us (window positions, cursor), so it
//! copies our DPI awareness
const wchar_t *dpiArgument()
{
  switch (GetAwarenessFromDpiAwarenessContext(GetThreadDpiAwarenessContext())) {
  case DPI_AWARENESS_UNAWARE:
    return L" --dpi unaware";
  case DPI_AWARENESS_SYSTEM_AWARE:
    return L" --dpi system";
  default:
    return L" --dpi permonitor";
  }
}

std::wstring helperPath()
{
  std::wstring module(MAX_PATH, L'\0');
  while (true) {
    const DWORD n = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
    if (n == 0) {
      return {};
    }
    if (n < module.size()) {
      module.resize(n);
      break;
    }
    module.resize(module.size() * 2);
  }
  return (std::filesystem::path(module).parent_path() / L"deskflow-filedrag.exe").wstring();
}

} // namespace

MSWindowsFileDragBridge::MSWindowsFileDragBridge(bool primary) : m_primary(primary)
{
}

MSWindowsFileDragBridge::~MSWindowsFileDragBridge()
{
  stop();
}

bool MSWindowsFileDragBridge::start(Handler handler)
{
  m_handler = std::move(handler);
  m_mainThread = GetCurrentThreadId();

  const std::wstring exe = helperPath();
  std::error_code ec;
  if (exe.empty() || !std::filesystem::exists(exe, ec)) {
    LOG_INFO("file drag is off: deskflow-filedrag.exe was not found next to deskflow-core");
    return false;
  }

  // OLE drag and drop only works between programs of the same integrity level, so the
  // helper must run as the signed-in user, not with our (possibly elevated) token
  HANDLE userToken = nullptr;
  const wchar_t *desktop = nullptr;
  if (isLocalSystem()) {
    try {
      MSWindowsSession session;
      session.updateActiveSession();
      userToken = session.getUserToken(nullptr);
      desktop = L"winsta0\\default";
    } catch (const std::runtime_error &e) {
      LOG_WARN("file drag is off: no signed-in user to run the helper as (%s)", e.what());
      return false;
    }
  } else if (ArchMiscWindows::isProcessElevated()) {
    LOG_WARN("file drag is off: Deskflow runs elevated without its service, so drag and drop cannot reach it");
    return false;
  }

  std::string error;
  const bool started = pipe::startHelper(
      exe, std::wstring(m_primary ? L"--primary" : L"--secondary") + dpiArgument(), userToken, desktop, m_process, error
  );
  if (userToken != nullptr) {
    CloseHandle(userToken);
  }
  if (!started) {
    LOG_WARN("file drag is off: cannot start its helper: %s", error.c_str());
    return false;
  }

  m_running = true;
  m_stopWriter = false;
  m_writer = std::thread([this] { writerLoop(); });
  m_reader = std::thread([this] { readerLoop(); });

  // wait for its hello (a fresh helper needs well under a second)
  std::unique_lock lock(m_syncMutex);
  if (!m_syncCond.wait_for(lock, std::chrono::seconds(5), [this] { return m_hello || !m_running; }) || !m_hello) {
    lock.unlock();
    LOG_WARN("file drag is off: its helper did not start");
    stop();
    return false;
  }
  LOG_INFO("file drag helper running, pid %lu, capabilities 0x%x", m_process.pid, m_capabilities.load());
  return true;
}

void MSWindowsFileDragBridge::stop()
{
  if (m_process.process == nullptr && !m_reader.joinable() && !m_writer.joinable()) {
    return;
  }
  // ask nicely (after anything still queued), then make sure
  send(Frame::Shutdown);
  m_running = false;
  {
    std::lock_guard lock(m_outMutex);
    m_stopWriter = true;
  }
  m_outCond.notify_all();
  if (m_process.process != nullptr && WaitForSingleObject(m_process.process, 1000) == WAIT_TIMEOUT) {
    TerminateProcess(m_process.process, 0);
    WaitForSingleObject(m_process.process, 1000);
  }
  // the helper is gone, so both pipes are broken and the threads return
  if (m_writer.joinable()) {
    m_writer.join();
  }
  if (m_reader.joinable()) {
    m_reader.join();
  }
  m_process.close();
  m_syncCond.notify_all();
}

void MSWindowsFileDragBridge::send(Frame frame, std::string payload)
{
  if (!m_running) {
    return;
  }
  {
    std::lock_guard lock(m_outMutex);
    m_out.emplace_back(frame, std::move(payload));
  }
  m_outCond.notify_one();
}

bool MSWindowsFileDragBridge::call(Frame frame, const std::string &payloadAfterToken, int timeoutMs)
{
  if (!m_running) {
    return false;
  }
  uint32_t token;
  {
    std::lock_guard lock(m_syncMutex);
    token = m_nextToken++;
  }
  ByteWriter writer;
  writer.u32(token);
  send(frame, writer.str() + payloadAfterToken);

  std::unique_lock lock(m_syncMutex);
  const bool acked = m_syncCond.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, token] {
    return m_syncAcked >= token || !m_running;
  });
  if (!acked || m_syncAcked < token) {
    LOG_WARN("file drag helper did not answer %s in time", pipe::frameName(frame));
    return false;
  }
  return true;
}

void MSWindowsFileDragBridge::dispatch()
{
  std::deque<std::pair<Frame, std::string>> frames;
  {
    std::lock_guard lock(m_inMutex);
    frames.swap(m_in);
  }
  for (auto &[frame, payload] : frames) {
    if (m_handler) {
      m_handler(frame, payload);
    }
  }
}

void MSWindowsFileDragBridge::writerLoop()
{
  pipe::FrameWriter writer(m_process.toHelper);
  while (true) {
    std::pair<Frame, std::string> item;
    {
      std::unique_lock lock(m_outMutex);
      m_outCond.wait(lock, [this] { return m_stopWriter || !m_out.empty(); });
      if (m_out.empty()) {
        return; // stopping
      }
      item = std::move(m_out.front());
      m_out.pop_front();
    }
    if (!writer.write(item.first, item.second)) {
      LOG_DEBUG("file drag helper pipe closed");
      m_running = false;
      m_syncCond.notify_all();
      std::lock_guard lock(m_outMutex);
      m_out.clear();
      if (m_stopWriter) {
        return;
      }
    }
  }
}

void MSWindowsFileDragBridge::readerLoop()
{
  Frame frame;
  std::string payload;
  while (pipe::readFrame(m_process.fromHelper, frame, payload)) {
    if (frame == Frame::Hello) {
      ByteReader reader(payload);
      uint32_t caps = 0;
      uint32_t pid = 0;
      reader.u32(caps);
      reader.u32(pid);
      m_capabilities = caps;
      std::lock_guard lock(m_syncMutex);
      m_hello = true;
      m_syncCond.notify_all();
      continue;
    }
    if (frame == Frame::SyncAck) {
      ByteReader reader(payload);
      uint32_t token = 0;
      if (reader.u32(token)) {
        std::lock_guard lock(m_syncMutex);
        if (token > m_syncAcked) {
          m_syncAcked = token;
        }
      }
      m_syncCond.notify_all();
      continue;
    }
    if (frame == Frame::Log) {
      ByteReader reader(payload);
      uint8_t level = 3;
      std::string text;
      reader.u8(level);
      reader.bytes32(text, 64 * 1024);
      switch (level) {
      case 0:
        LOG_ERR("file drag: %s", text.c_str());
        break;
      case 1:
        LOG_WARN("file drag: %s", text.c_str());
        break;
      case 2:
        LOG_INFO("file drag: %s", text.c_str());
        break;
      default:
        LOG_DEBUG("file drag: %s", text.c_str());
        break;
      }
      continue;
    }

    bool wake = false;
    {
      std::lock_guard lock(m_inMutex);
      wake = m_in.empty();
      m_in.emplace_back(frame, std::move(payload));
      payload.clear();
    }
    if (wake) {
      PostThreadMessageW(m_mainThread, DESKFLOW_MSG_FILEDRAG, 0, 0);
    }
  }

  if (m_running.exchange(false)) {
    LOG_WARN("file drag helper stopped; dragging files is off until Deskflow restarts");
  }
  m_syncCond.notify_all();
}
