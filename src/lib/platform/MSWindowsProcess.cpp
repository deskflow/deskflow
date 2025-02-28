/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsProcess.h"

#include "arch/win32/XArchWindows.h"
#include "base/Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <UserEnv.h>

#include <string>

namespace deskflow::platform {

MSWindowsProcess::MSWindowsProcess(const std::string &command, HANDLE stdOutput, HANDLE stdError)
    : m_command(command),
      m_stdOutput(stdOutput),
      m_stdError(stdError)
{
}

MSWindowsProcess::~MSWindowsProcess()
{
  if (m_createProcessResult) {
    CloseHandle(m_info.hProcess);
    CloseHandle(m_info.hThread);
  }
}

BOOL MSWindowsProcess::startInForeground()
{
  // clear, as we're reusing process info struct
  ZeroMemory(&m_info, sizeof(PROCESS_INFORMATION));

  // show the console window when in foreground mode,
  // so we can close it gracefully, but minimize it
  // so it doesn't get in the way.
  STARTUPINFO si;
  setStartupInfo(si);
  si.dwFlags |= STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_MINIMIZE;

  m_createProcessResult =
      CreateProcess(nullptr, LPSTR(m_command.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &m_info);
  return m_createProcessResult;
}

BOOL MSWindowsProcess::startAsUser(HANDLE userToken, LPSECURITY_ATTRIBUTES sa)
{
  STARTUPINFO si;
  setStartupInfo(si);

  LPVOID environment;
  if (!CreateEnvironmentBlock(&environment, userToken, FALSE)) {
    LOG((CLOG_ERR "could not create environment block"));
    throw XArch(new XArchEvalWindows);
  }

  ZeroMemory(&m_info, sizeof(PROCESS_INFORMATION));
  const DWORD creationFlags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;
  m_createProcessResult = CreateProcessAsUser(
      userToken, nullptr, LPSTR(m_command.c_str()), sa, nullptr, TRUE, creationFlags, environment, nullptr, &si, &m_info
  );

  DestroyEnvironmentBlock(environment);
  CloseHandle(userToken);

  return m_createProcessResult;
}

void MSWindowsProcess::setStartupInfo(STARTUPINFO &si)
{
  static char g_desktopName[] = "winsta0\\Default"; // NOSONAR -- Idiomatic Win32

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.lpDesktop = g_desktopName;
  si.hStdOutput = m_stdOutput;
  si.hStdError = m_stdError;
  si.dwFlags |= STARTF_USESTDHANDLES;
}

DWORD MSWindowsProcess::waitForExit()
{
  const auto kMaxWaitMilliseconds = 10000;

  LOG_INFO("waiting for process to exit, pid: %lu", m_info.dwProcessId);
  if (WaitForSingleObject(m_info.hProcess, kMaxWaitMilliseconds) != WAIT_OBJECT_0) {
    LOG_ERR("process did not exit within the expected time");
    TerminateProcess(m_info.hProcess, 1);
    throw XArch(new XArchEvalWindows());
  }

  DWORD exitCode = 0;
  if (!GetExitCodeProcess(m_info.hProcess, &exitCode)) {
    LOG_ERR("failed to get exit code, error: %lu", GetLastError());
    throw XArch(new XArchEvalWindows());
  }

  if (exitCode != 0) {
    LOG_WARN("process failed with exit code: %lu", exitCode);
  } else {
    LOG_DEBUG("process exited with code: %lu", exitCode);
  }

  return exitCode;
}

void MSWindowsProcess::shutdown(int timeout)
{
  shutdown(m_info.hProcess, m_info.dwProcessId, timeout);
}

void MSWindowsProcess::shutdown(HANDLE handle, DWORD pid, int timeout)
{
  LOG_DEBUG("shutting down process %d", pid);

  LOG_DEBUG("sending close event to close process gracefully");
  HANDLE hCloseEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, deskflow::common::kCloseEventName);
  if (hCloseEvent != nullptr) { // NOSONAR -- Readability
    SetEvent(hCloseEvent);
    CloseHandle(hCloseEvent);
  } else {
    // This is expected to fail when no server/client is running.
    LOG_DEBUG("could not send close event to process");
  }

  DWORD exitCode;
  if (GetExitCodeProcess(handle, &exitCode)) {
    if (exitCode == STILL_ACTIVE) {
      LOG_DEBUG("process %d is still running", pid);
    } else {
      // Don't bother shutting down a process that has already exited.
      LOG_DEBUG("process %d is already shutdown", pid);
      return;
    }
  } else {
    XArchEvalWindows error;
    LOG_ERR("failed to get process exit code for process %d, error:", pid, error.eval().c_str());
  }

  // Wait for process to exit gracefully.
  LOG_DEBUG("waiting for process %d to exit gracefully", pid);
  DWORD waitResult = WaitForSingleObject(handle, timeout * 1000);
  if (waitResult == WAIT_OBJECT_0) { // NOSONAR - Readability
    if (!GetExitCodeProcess(handle, &exitCode)) {
      XArchEvalWindows error;
      LOG_ERR("failed to retrieve exit code after process exit for process %d, error:", pid, error.eval().c_str());
    }

    LOG_DEBUG("process %d was shutdown gracefully with exit code %d", pid, exitCode);
    return;

  } else if (waitResult == WAIT_TIMEOUT) {
    LOG_WARN("process %d did not exit within the expected time", pid);
  } else {
    XArchEvalWindows error;
    LOG_ERR("error waiting for process %d to exit, error:", pid, error.eval().c_str());
  }

  // Last resort, terminate the process forcefully.
  if (TerminateProcess(handle, kExitSuccess)) {
    LOG_WARN("forcefully terminated process %d", pid);
  } else {
    XArchEvalWindows error;
    LOG_ERR("failed to terminate process %d, error:", pid, error.eval().c_str());
  }
}

void MSWindowsProcess::createPipes()
{
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  if (!CreatePipe(&m_outputPipe, &m_stdOutput, &saAttr, 0)) {
    LOG_ERR("could not create output pipe");
    throw XArch(new XArchEvalWindows());
  }

  if (!CreatePipe(&m_errorPipe, &m_stdError, &saAttr, 0)) {
    LOG_ERR("could not create error pipe");
    throw XArch(new XArchEvalWindows());
  }

  // Set the pipes to non-blocking mode
  DWORD mode = PIPE_NOWAIT;
  if (!SetNamedPipeHandleState(m_outputPipe, &mode, nullptr, nullptr)) {
    throw XArch(new XArchEvalWindows());
  }

  if (!SetNamedPipeHandleState(m_errorPipe, &mode, nullptr, nullptr)) {
    throw XArch(new XArchEvalWindows());
  }
}

std::string MSWindowsProcess::readStdOutput()
{
  return readOutput(m_outputPipe);
}

std::string MSWindowsProcess::readStdError()
{
  return readOutput(m_errorPipe);
}

std::string MSWindowsProcess::readOutput(HANDLE handle)
{
  const auto kOutputBufferSize = 4096;
  char buffer[kOutputBufferSize]; // NOSONAR -- Idiomatic Win32

  DWORD bytesRead;
  DWORD totalBytesAvail;
  DWORD bytesLeftThisMessage;

  // Check if there is data available in the pipe, which prevents `ReadFile` from freezing execution.
  if (!PeekNamedPipe(handle, nullptr, 0, nullptr, &totalBytesAvail, &bytesLeftThisMessage)) {
    LOG_ERR("could not peek into pipe");
    throw XArch(new XArchEvalWindows());
  }

  if (totalBytesAvail == 0) {
    return "";
  }

  if (!ReadFile(handle, buffer, kOutputBufferSize, &bytesRead, nullptr)) {
    LOG_ERR("could not read from pipe");
    throw XArch(new XArchEvalWindows());
  }

  return std::string(buffer, bytesRead);
}

} // namespace deskflow::platform
