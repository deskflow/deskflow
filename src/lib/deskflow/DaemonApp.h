/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ipc/IpcServer.h"

#include <memory>
#include <string>

#include <QCoreApplication>

class Event;
class IpcLogOutputter;
class FileLogOutputter;
class QLocalServer;

namespace deskflow::ipc {
class IpcServer2;
}

#if SYSAPI_WIN32
class MSWindowsWatchdog;
#endif

extern const char *const kLogFilename;

class DaemonApp : public QCoreApplication
{
public:
  DaemonApp(IEventQueue *events, int argc, char **argv);
  ~DaemonApp();
  void startAsync();
  void mainLoop(bool logToFile, bool foreground = false);

private:
  int init(int argc, char **argv);
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
  IEventQueue *m_events = nullptr;
  FileLogOutputter *m_fileLogOutputter = nullptr;
  std::unique_ptr<deskflow::ipc::IpcServer2> m_ipcServer2;
};
