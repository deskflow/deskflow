/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

namespace deskflow::platform {

namespace { // NOSONAR -- Deliberate anonymous
const auto kDefaultShutdownTimeout = 10;
}

class MSWindowsProcess
{
public:
  explicit MSWindowsProcess(const std::wstring &command, HANDLE stdOutput = nullptr, HANDLE stdError = nullptr);
  ~MSWindowsProcess();

  BOOL startInForeground();
  BOOL startAsUser(HANDLE userToken, LPSECURITY_ATTRIBUTES sa);
  void shutdown(int timeout = kDefaultShutdownTimeout);
  DWORD waitForExit();
  void createPipes();
  std::wstring readStdOutput();
  std::wstring readStdError();

  PROCESS_INFORMATION info() const
  {
    return m_info;
  }

  static void shutdown(HANDLE handle, DWORD pid, int timeout = kDefaultShutdownTimeout);

private:
  void setStartupInfo(STARTUPINFO &si);

  static std::wstring readOutput(HANDLE handle);

  std::wstring m_command;
  HANDLE m_stdOutput;
  HANDLE m_stdError;
  HANDLE m_outputPipe = nullptr;
  HANDLE m_errorPipe = nullptr;
  PROCESS_INFORMATION m_info;
  BOOL m_createProcessResult = FALSE;
};

} // namespace deskflow::platform
