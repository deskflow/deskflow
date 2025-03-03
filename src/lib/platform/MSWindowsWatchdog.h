/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsProcess.h"
#include "platform/MSWindowsSession.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include <optional>
#include <string>

typedef VOID(WINAPI *SendSas)(BOOL asUser);

class Thread;
class FileLogOutputter;

class MSWindowsWatchdog
{
  enum class ProcessState
  {
    Idle,
    StartScheduled,
    StartPending,
    StopPending,
    Running
  };

public:
  explicit MSWindowsWatchdog(bool foreground);
  ~MSWindowsWatchdog() = default;

  void startAsync();
  void setProcessConfig(const std::string_view &command, bool elevate);
  void stop();
  bool isProcessRunning();
  void setFileLogOutputter(FileLogOutputter *outputter);

private:
  void mainLoop(void *);
  void outputLoop(void *);
  void shutdownExistingProcesses();
  HANDLE duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security);
  HANDLE getUserToken(LPSECURITY_ATTRIBUTES security, bool elevatedToken);
  void startProcess();
  void setStartupInfo(STARTUPINFO &si);
  void handleStartError(const std::string_view &message = "");
  void initOutputReadPipe();
  void initSasFunc();
  void sendSas() const;

  /**
   * @brief Re-run the process to get the active desktop name.
   *
   * It is necessary to run a utility process because the daemon runs in session 0, which does not
   * have access to the active desktop, and so cannot query it's name.
   *
   * @return std::string The name of the active desktop.
   */
  std::string runActiveDesktopUtility();

  static std::string processStateToString(ProcessState state);

private:
  Thread *m_thread = nullptr;
  bool m_running = true;
  HANDLE m_outputWritePipe = nullptr;
  HANDLE m_outputReadPipe = nullptr;
  Thread *m_outputThread = nullptr;
  bool m_elevateProcess = false;
  MSWindowsSession m_session;
  int m_startFailures = 0;
  FileLogOutputter *m_fileLogOutputter = nullptr;
  bool m_foreground;
  std::string m_activeDesktop;
  std::unique_ptr<deskflow::platform::MSWindowsProcess> m_process;
  std::optional<double> m_nextStartTime = std::nullopt;
  ProcessState m_processState = ProcessState::Idle;
  std::string m_command = "";
  SendSas m_sendSasFunc = nullptr;
};
