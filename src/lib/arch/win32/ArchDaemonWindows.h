/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchDaemon.h"
#include "arch/IArchMultithread.h"
#include "common/constants.h"
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>

#define ARCH_DAEMON ArchDaemonWindows

//! Win32 implementation of IArchDaemon
class ArchDaemonWindows : public IArchDaemon
{
public:
  typedef int (*RunFunc)(void);

  ArchDaemonWindows();
  virtual ~ArchDaemonWindows();

  //! Run the daemon
  /*!
  When the client calls \c daemonize(), the \c DaemonFunc should call this
  function after initialization and argument parsing to perform the
  daemon processing.  The \c runFunc should perform the daemon's
  main loop, calling \c daemonRunning(true) when it enters the main loop
  (i.e. after initialization) and \c daemonRunning(false) when it leaves
  the main loop.  The \c runFunc is called in a new thread and when the
  daemon must exit the main loop due to some external control the
  getDaemonQuitMessage() is posted to the thread.  This function returns
  what \c runFunc returns.  \c runFunc should call \c daemonFailed() if
  the daemon fails.
  */
  static int runDaemon(RunFunc runFunc);

  //! Indicate daemon is in main loop
  /*!
  The \c runFunc passed to \c runDaemon() should call this function
  to indicate when it has entered (\c running is \c true) or exited
  (\c running is \c false) the main loop.
  */
  static void daemonRunning(bool running);

  //! Indicate failure of running daemon
  /*!
  The \c runFunc passed to \c runDaemon() should call this function
  to indicate failure.  \c result is returned by \c daemonize().
  */
  static void daemonFailed(int result);

  //! Get daemon quit message
  /*!
  The windows NT daemon tells daemon thread to exit by posting this
  message to it.  The thread must, of course, have a message queue
  for this to work.
  */
  static UINT getDaemonQuitMessage();

  // IArchDaemon overrides
  virtual void installDaemon(
      const char *name, const char *description, const char *pathname, const char *commandLine, const char *dependencies
  );
  virtual void uninstallDaemon(const char *name);
  virtual void installDaemon();
  virtual void uninstallDaemon();
  virtual int daemonize(const char *name, DaemonFunc func);
  virtual bool canInstallDaemon(const char *name);
  virtual bool isDaemonInstalled(const char *name);
  std::string commandLine() const
  {
    return m_commandLine;
  }

private:
  static HKEY openNTServicesKey();

  int doRunDaemon(RunFunc runFunc);
  void doDaemonRunning(bool running);
  UINT doGetDaemonQuitMessage();

  static void setStatus(DWORD state);
  static void setStatus(DWORD state, DWORD step, DWORD waitHint);
  static void setStatusError(DWORD error);

  static bool isRunState(DWORD state);

  void serviceMain(DWORD, LPTSTR *);
  static void WINAPI serviceMainEntry(DWORD, LPTSTR *);

  void serviceHandler(DWORD ctrl);
  static void WINAPI serviceHandlerEntry(DWORD ctrl);

  void start(const char *name);
  void stop(const char *name);

private:
  class XArchDaemonRunFailed
  {
  public:
    XArchDaemonRunFailed(int result) : m_result(result)
    {
    }

  public:
    int m_result;
  };

private:
  static ArchDaemonWindows *s_daemon;

  ArchMutex m_serviceMutex;
  ArchCond m_serviceCondVar;
  DWORD m_serviceState;
  bool m_serviceHandlerWaiting;
  bool m_serviceRunning;

  DWORD m_daemonThreadID;
  DaemonFunc m_daemonFunc;
  int m_daemonResult;

  SERVICE_STATUS_HANDLE m_statusHandle;

  UINT m_quitMessage;

  std::string m_commandLine;
};

#define DEFAULT_DAEMON_NAME _T(kAppName)
#define DEFAULT_DAEMON_INFO _T("Manages the Deskflow foreground processes.")

#define LEGACY_SERVER_DAEMON_NAME _T("Deskflow Server")
#define LEGACY_CLIENT_DAEMON_NAME _T("Deskflow Client")
