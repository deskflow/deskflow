/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ipc/IpcServer.h"

#include <memory>
#include <string>

#include <QObject>

class Event;
class IpcLogOutputter;
class FileLogOutputter;
class QLocalServer;
class QCoreApplication;

namespace deskflow::core::ipc {
class DaemonIpcServer;
}

#if SYSAPI_WIN32
class MSWindowsWatchdog;
#endif

extern const char *const kLogFilename;

class DaemonApp : public QObject
{
  Q_OBJECT

public:
  DaemonApp(QCoreApplication *app);
  ~DaemonApp();
  void init(int argc, char **argv);
  void run();
  void mainLoop(bool logToFile, bool foreground = false);

signals:
  void fatalError();
  void serviceInstalled();
  void serviceUninstalled();

private:
  void daemonize();
  void foregroundError(const char *message);
  std::string logFilename();
  void handleIpcMessage(const Event &, void *);

private slots:
  void handleElevateModeChanged(int mode);
  void handleCommandChanged(const QString &command);
  void handleRestartRequested();

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
  deskflow::core::ipc::DaemonIpcServer *m_ipcServer2 = nullptr;
  std::string m_command = "";
  int m_elevateMode = 0;
  bool m_foreground = false;
};
