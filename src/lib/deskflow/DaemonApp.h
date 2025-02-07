/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

#include <memory>
#include <string>

#include <QObject>

#include "common/common.h"

class Event;
class IEventQueue;
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
  void init(IEventQueue *events, int argc, char **argv);
  void run();
  void mainLoop(bool logToFile, bool foreground = false);

  static DaemonApp &instance()
  {
    static DaemonApp instance; // NOSONAR - Meyers' Singleton
    return instance;
  }

signals:
  void mainLoopFinished();
  void fatalErrorOccurred();
  void serviceInstalled();
  void serviceUninstalled();

private:
  explicit DaemonApp();
  ~DaemonApp() override;

  void daemonize();
  void foregroundError(const char *message);
  std::string logFilename();
  void handleIpcMessage(const Event &, void *);

private slots:
  void handleElevateModeChanged(int mode);
  void handleCommandChanged(const QString &command);
  void handleRestartRequested();

#if SYSAPI_WIN32
  std::unique_ptr<MSWindowsWatchdog> m_watchdog;
#endif

private:
  std::unique_ptr<IpcLogOutputter> m_ipcLogOutputter;
  IEventQueue *m_events = nullptr;
  FileLogOutputter *m_fileLogOutputter = nullptr;
  deskflow::core::ipc::DaemonIpcServer *m_ipcServer = nullptr;
  std::string m_command = "";
  int m_elevateMode = 0;
  bool m_foreground = false;
};
