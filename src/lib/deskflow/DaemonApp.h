/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ipc/IpcServer.h"

#include <memory>
#include <string>

class Event;
class IpcLogOutputter;
class FileLogOutputter;

#if SYSAPI_WIN32
class MSWindowsWatchdog;
#endif

extern const char *const kLogFilename;

class DaemonApp
{

public:
  DaemonApp();
  ~DaemonApp();
  int run(int argc, char **argv);
  void mainLoop(bool logToFile, bool foreground = false);

private:
  void daemonize();
  void foregroundError(const char *message);
  std::string logFilename();
  void handleIpcMessage(const Event &, void *);

public:
  static DaemonApp *s_instance;

#if SYSAPI_WIN32
  std::unique_ptr<MSWindowsWatchdog> m_watchdog;
#endif

private:
  std::unique_ptr<IpcServer> m_ipcServer;
  std::unique_ptr<IpcLogOutputter> m_ipcLogOutputter;
  std::unique_ptr<IEventQueue> m_events;
  std::unique_ptr<FileLogOutputter> m_fileLogOutputter;
};
