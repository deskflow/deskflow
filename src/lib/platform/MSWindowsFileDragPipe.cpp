/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsFileDragPipe.h"

#include <userenv.h>

#include <vector>

namespace deskflow::filedrag::pipe {

const char *frameName(Frame frame)
{
  switch (frame) {
    using enum Frame;
  case WatchEdges:
    return "WatchEdges";
  case HandOff:
    return "HandOff";
  case TakeBack:
    return "TakeBack";
  case Peer:
    return "Peer";
  case Shutdown:
    return "Shutdown";
  case Entered:
    return "Entered";
  case Hello:
    return "Hello";
  case OfferReady:
    return "OfferReady";
  case OfferLost:
    return "OfferLost";
  case LauncherReady:
    return "LauncherReady";
  case LoopEnded:
    return "LoopEnded";
  case SyncAck:
    return "SyncAck";
  case Log:
    return "Log";
  case TestSourceEnter:
    return "TestSourceEnter";
  case TestSourceLeave:
    return "TestSourceLeave";
  case TestDropTarget:
    return "TestDropTarget";
  case TestRelease:
    return "TestRelease";
  case TestEvent:
    return "TestEvent";
  }
  return "?";
}

//! CreateProcessAsUserW needs these held and enabled (a SYSTEM token holds them)
static void enablePrivileges()
{
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
    return;
  }
  for (const wchar_t *name : {L"SeAssignPrimaryTokenPrivilege", L"SeIncreaseQuotaPrivilege"}) {
    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid)) {
      AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    }
  }
  CloseHandle(token);
}

static bool writeAll(HANDLE pipe, const char *data, size_t size)
{
  while (size > 0) {
    DWORD written = 0;
    const DWORD part = static_cast<DWORD>(size > (1u << 20) ? (1u << 20) : size);
    if (!WriteFile(pipe, data, part, &written, nullptr) || written == 0) {
      return false;
    }
    data += written;
    size -= written;
  }
  return true;
}

static bool readAll(HANDLE pipe, char *data, size_t size)
{
  while (size > 0) {
    DWORD got = 0;
    const DWORD part = static_cast<DWORD>(size > (1u << 20) ? (1u << 20) : size);
    if (!ReadFile(pipe, data, part, &got, nullptr) || got == 0) {
      return false;
    }
    data += got;
    size -= got;
  }
  return true;
}

bool FrameWriter::write(Frame frame, std::string_view payload)
{
  if (m_pipe == nullptr || payload.size() + 1 > kMaxFrameBytes) {
    return false;
  }
  const auto length = static_cast<uint32_t>(payload.size() + 1);
  std::string buffer;
  buffer.reserve(payload.size() + 5);
  buffer.push_back(static_cast<char>((length >> 24) & 0xff));
  buffer.push_back(static_cast<char>((length >> 16) & 0xff));
  buffer.push_back(static_cast<char>((length >> 8) & 0xff));
  buffer.push_back(static_cast<char>(length & 0xff));
  buffer.push_back(static_cast<char>(frame));
  buffer.append(payload);

  std::lock_guard lock(m_mutex);
  return writeAll(m_pipe, buffer.data(), buffer.size());
}

bool readFrame(HANDLE pipe, Frame &frame, std::string &payload)
{
  unsigned char header[5];
  if (!readAll(pipe, reinterpret_cast<char *>(header), sizeof(header))) {
    return false;
  }
  const uint32_t length =
      (uint32_t(header[0]) << 24) | (uint32_t(header[1]) << 16) | (uint32_t(header[2]) << 8) | uint32_t(header[3]);
  if (length == 0 || length > kMaxFrameBytes) {
    return false;
  }
  frame = static_cast<Frame>(header[4]);
  payload.resize(length - 1);
  return length == 1 || readAll(pipe, payload.data(), payload.size());
}

void HelperProcess::close()
{
  for (HANDLE *h : {&toHelper, &fromHelper, &process, &job}) {
    if (*h != nullptr) {
      CloseHandle(*h);
      *h = nullptr;
    }
  }
  pid = 0;
}

bool startHelper(
    const std::wstring &exe, const std::wstring &extraArgs, HANDLE userToken, const wchar_t *desktop,
    HelperProcess &out, std::string &error
)
{
  out = HelperProcess{};

  // pipe ends for the helper are inheritable, ours are not
  SECURITY_ATTRIBUTES inherit = {sizeof(inherit), nullptr, TRUE};
  HANDLE helperIn = nullptr;  // helper reads
  HANDLE coreOut = nullptr;   // core writes
  HANDLE coreIn = nullptr;    // core reads
  HANDLE helperOut = nullptr; // helper writes
  constexpr DWORD kPipeBuffer = 1u << 20;
  if (!CreatePipe(&helperIn, &coreOut, &inherit, kPipeBuffer) ||
      !CreatePipe(&coreIn, &helperOut, &inherit, kPipeBuffer)) {
    error = "CreatePipe: " + errorText(GetLastError());
    for (HANDLE h : {helperIn, coreOut, coreIn, helperOut}) {
      if (h != nullptr) {
        CloseHandle(h);
      }
    }
    return false;
  }
  SetHandleInformation(coreOut, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(coreIn, HANDLE_FLAG_INHERIT, 0);

  auto closeHelperEnds = [&] {
    CloseHandle(helperIn);
    CloseHandle(helperOut);
  };
  auto fail = [&](const char *what) {
    error = std::string(what) + ": " + errorText(GetLastError());
    closeHelperEnds();
    CloseHandle(coreOut);
    CloseHandle(coreIn);
    return false;
  };

  // only the two helper ends may be inherited
  SIZE_T attrSize = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
  std::vector<char> attrBuffer(attrSize);
  auto *attrs = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrBuffer.data());
  if (!InitializeProcThreadAttributeList(attrs, 1, 0, &attrSize)) {
    return fail("InitializeProcThreadAttributeList");
  }
  HANDLE inheritList[2] = {helperIn, helperOut};
  if (!UpdateProcThreadAttribute(
          attrs, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inheritList, sizeof(inheritList), nullptr, nullptr
      )) {
    DeleteProcThreadAttributeList(attrs);
    return fail("UpdateProcThreadAttribute");
  }

  std::wstring command = L"\"" + exe + L"\" --pipe-in " +
                         std::to_wstring(static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(helperIn))) +
                         L" --pipe-out " +
                         std::to_wstring(static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(helperOut)));
  if (!extraArgs.empty()) {
    command += L" " + extraArgs;
  }

  STARTUPINFOEXW si = {};
  si.StartupInfo.cb = sizeof(si);
  si.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  si.StartupInfo.wShowWindow = SW_HIDE;
  std::wstring desktopName = desktop != nullptr ? desktop : L"";
  if (!desktopName.empty()) {
    si.StartupInfo.lpDesktop = desktopName.data();
  }
  si.lpAttributeList = attrs;

  const DWORD flags = EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT;
  PROCESS_INFORMATION pi = {};
  BOOL started = FALSE;
  if (userToken != nullptr) {
    enablePrivileges();
    LPVOID environment = nullptr;
    if (!CreateEnvironmentBlock(&environment, userToken, FALSE)) {
      environment = nullptr;
    }
    started = CreateProcessAsUserW(
        userToken, exe.c_str(), command.data(), nullptr, nullptr, TRUE, flags, environment, nullptr, &si.StartupInfo,
        &pi
    );
    const DWORD lastError = GetLastError();
    if (environment != nullptr) {
      DestroyEnvironmentBlock(environment);
    }
    SetLastError(lastError);
  } else {
    started = CreateProcessW(
        exe.c_str(), command.data(), nullptr, nullptr, TRUE, flags, nullptr, nullptr, &si.StartupInfo, &pi
    );
  }
  DeleteProcThreadAttributeList(attrs);
  if (!started) {
    return fail(userToken != nullptr ? "CreateProcessAsUserW" : "CreateProcessW");
  }
  closeHelperEnds();

  // end the helper together with us
  out.job = CreateJobObjectW(nullptr, nullptr);
  if (out.job != nullptr) {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(out.job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) ||
        !AssignProcessToJobObject(out.job, pi.hProcess)) {
      // not fatal: the helper also exits when its pipe breaks
      CloseHandle(out.job);
      out.job = nullptr;
    }
  }
  ResumeThread(pi.hThread);
  CloseHandle(pi.hThread);

  out.process = pi.hProcess;
  out.pid = pi.dwProcessId;
  out.toHelper = coreOut;
  out.fromHelper = coreIn;
  return true;
}

std::wstring widen(std::string_view utf8)
{
  if (utf8.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
  return out;
}

std::string narrow(std::wstring_view wide)
{
  if (wide.empty()) {
    return {};
  }
  const int n =
      WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), n, nullptr, nullptr);
  return out;
}

std::string errorText(DWORD code)
{
  wchar_t *buffer = nullptr;
  const DWORD n = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
      reinterpret_cast<LPWSTR>(&buffer), 0, nullptr
  );
  std::string text = n > 0 ? narrow(std::wstring_view(buffer, n)) : std::string();
  if (buffer != nullptr) {
    LocalFree(buffer);
  }
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ' || text.back() == '.')) {
    text.pop_back();
  }
  return text + " (" + std::to_string(code) + ")";
}

} // namespace deskflow::filedrag::pipe
