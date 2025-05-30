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
#include <mutex>
#include <optional>
#include <string>

typedef VOID(WINAPI *SendSas)(BOOL asUser);

class Thread;
class FileLogOutputter;

/**
 * @brief Monitors and controls a core process on Windows, elevating if necessary.
 */
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
  explicit MSWindowsWatchdog(bool foreground, FileLogOutputter &fileLogOutputter);
  ~MSWindowsWatchdog() = default;

  /**
   * @brief Start threads for main loop and and output loop.
   */
  void startAsync();

  /**
   * @brief Set the command to run and whether to elevate the process.
   */
  void setProcessConfig(const std::string_view &command, bool elevate);

  /**
   * @brief Stop the main loop and output loop threads.
   */
  void stop();

  /**
   * @return True if the process is running.
   */
  bool isProcessRunning();

private:
  /**
   * @brief Monitor the process state and start/stop the process as necessary.
   */
  void mainLoop(void *);

  /**
   * @brief Monitor the process standard out/error and write to the file log outputter.
   */
  void outputLoop(void *);

  /**
   * @brief Duplicates the process token for the given process.
   *
   * Required for starting a process in the user session; when we start an elevated process
   * to ensure that it has access to secure processes, such as the login screen, we duplicate
   * the token of an existing process that has the necessary access such as `winlogon.exe`.
   *
   * @param process The process to duplicate the token from (typically `winlogon.exe`).
   */
  HANDLE duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security);

  /**
   * @brief Get a security token for the user session.
   *
   * Checks to see if logonui.exe is running or if the `elevatedToken` arg is true,
   * which indicates either we're in a secure user session or we need an elevated token.
   * If either case is true, it duplicates the token from `winlogon.exe`.
   */
  HANDLE getUserToken(LPSECURITY_ATTRIBUTES security, bool elevatedToken);

  /**
   * @brief Start the core process, elevating if necessary.
   */
  void startProcess();

  /**
   * @brief Controls whether the process should restart immediately or delay start.
   */
  ProcessState handleStartError(const std::string_view &message = "");

  /**
   * @brief Init the output read pipe for standard out/error.
   */
  void initOutputReadPipe();

  /**
   * @brief Init the SendSAS function, used for Ctrl+Alt+Del emulation.
   */
  void initSasFunc();

  /**
   * @brief Allows the SendSAS function to be called from other processes.
   *
   * SendSAS sends a SAS (Secure Attention Sequence) for Ctrl+Alt+Del emulation.
   */
  void sasLoop(void *);

  /**
   * @brief Convert the process state enum to a string (useful for logging).
   */
  static std::string processStateToString(ProcessState state);

  /**
   * @brief Stops any core processes which were not started by the watchdog.
   */
  static void shutdownExistingProcesses();

private:
  bool m_running = true;
  std::unique_ptr<Thread> m_mainThread;
  std::unique_ptr<Thread> m_outputThread;
  std::unique_ptr<Thread> m_sasThread;
  HANDLE m_outputWritePipe = nullptr;
  HANDLE m_outputReadPipe = nullptr;
  bool m_elevateProcess = false;
  MSWindowsSession m_session;
  int m_startFailures = 0;
  FileLogOutputter &m_fileLogOutputter;
  bool m_foreground = false;
  std::wstring m_activeDesktop = {};
  std::unique_ptr<deskflow::platform::MSWindowsProcess> m_process;
  std::optional<double> m_nextStartTime = std::nullopt;
  ProcessState m_processState = ProcessState::Idle;
  std::wstring m_command = {};
  SendSas m_sendSasFunc = nullptr;
  std::mutex m_processStateMutex;
};
