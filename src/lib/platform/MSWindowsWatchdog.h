/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"
#include "deskflow/XDeskflow.h"
#include "platform/MSWindowsProcess.h"
#include "platform/MSWindowsSession.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include <string>

class Thread;
class FileLogOutputter;

class MSWindowsWatchdog
{
public:
  MSWindowsWatchdog(bool autoDetectCommand, bool foreground);
  virtual ~MSWindowsWatchdog();

  void startAsync();
  std::string getCommand() const;
  void setCommand(const std::string &command, bool elevate);
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
  void sendSas();
  void setStartupInfo(STARTUPINFO &si);

  /**
   * @brief Re-run the process to get the active desktop name.
   *
   * It is necessary to run a utility process because the daemon runs in session 0, which does not
   * have access to the active desktop, and so cannot query it's name.
   *
   * @return std::string The name of the active desktop.
   */
  std::string runActiveDesktopUtility();

private:
  Thread *m_thread;
  bool m_autoDetectCommand;
  std::string m_command;
  bool m_monitoring;
  bool m_commandChanged;
  HANDLE m_outputWritePipe;
  HANDLE m_outputReadPipe;
  Thread *m_outputThread;
  bool m_elevateProcess;
  MSWindowsSession m_session;
  int m_processFailures;
  bool m_processStarted;
  FileLogOutputter *m_fileLogOutputter;
  ArchMutex m_mutex;
  ArchCond m_condVar;
  bool m_ready;
  bool m_foreground;
  std::string m_activeDesktop;
  std::unique_ptr<deskflow::platform::MSWindowsProcess> m_process;
};

//! Relauncher error
/*!
An error occured in the process watchdog.
*/
class XMSWindowsWatchdogError : public XDeskflow
{
public:
  XMSWindowsWatchdogError(const std::string &msg) : XDeskflow(msg)
  {
  }

  // XBase overrides
  virtual std::string getWhat() const throw()
  {
    return what();
  }
};
