/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2009 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"
#include "deskflow/XDeskflow.h"
#include "platform/MSWindowsSession.h"

#include <map>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Thread;
class IpcLogOutputter;
class IpcServer;
class FileLogOutputter;

class MSWindowsWatchdog
{
public:
  MSWindowsWatchdog(bool autoDetectCommand, IpcServer &ipcServer, IpcLogOutputter &ipcLogOutputter, bool foreground);
  virtual ~MSWindowsWatchdog();

  void startAsync();
  std::string getCommand() const;
  void setCommand(const std::string &command, bool elevate);
  void stop();
  bool isProcessActive();
  void setFileLogOutputter(FileLogOutputter *outputter);

private:
  void mainLoop(void *);
  void outputLoop(void *);
  void shutdownProcess(HANDLE handle, DWORD pid, int timeout);
  void shutdownExistingProcesses();
  HANDLE duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security);
  HANDLE getUserToken(LPSECURITY_ATTRIBUTES security);
  void startProcess();
  BOOL startProcessAsUser(std::string &command, HANDLE userToken, LPSECURITY_ATTRIBUTES sa);
  BOOL startProcessInForeground(std::string &command);
  void sendSas();
  void getActiveDesktop(LPSECURITY_ATTRIBUTES security);
  void testOutput(std::string buffer);
  void setStartupInfo(STARTUPINFO &si);
  void checkChildren();
  /**
   * @brief This closes the handles held to a child thread
   * @param pid the ID of the process to kill, will do nothing if PID is not a
   * valid child
   * @param removeFromMap should the function remove the item from the children
   * map
   */
  void closeProcessHandles(unsigned long pid, bool removeFromMap = true);
  /**
   * @brief This kills off all children's handles created by this process
   */
  void clearAllChildren();

private:
  Thread *m_thread;
  bool m_autoDetectCommand;
  std::string m_command;
  bool m_monitoring;
  bool m_commandChanged;
  HANDLE m_stdOutWrite;
  HANDLE m_stdOutRead;
  Thread *m_outputThread;
  IpcServer &m_ipcServer;
  IpcLogOutputter &m_ipcLogOutputter;
  bool m_elevateProcess;
  MSWindowsSession m_session;
  PROCESS_INFORMATION m_processInfo;
  int m_processFailures;
  bool m_processRunning;
  FileLogOutputter *m_fileLogOutputter;
  bool m_autoElevated;
  ArchMutex m_mutex;
  ArchCond m_condVar;
  bool m_ready;
  bool m_foreground;
  std::string m_activeDesktop;

  /// @brief Save the info of all process made
  ///         We will use this to track all processes we make and
  ///         kill off handels and children that we no longer need
  std::map<unsigned long, PROCESS_INFORMATION> m_children;
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
