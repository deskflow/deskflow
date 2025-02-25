/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

#include <memory>
#include <string>

#include <QObject>

class Event;
class IEventQueue;
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
  enum class InitResult
  {
    Installed,
    Uninstalled,
    StartDaemon,
    ShowHelp,
    ArgsError,
    FatalError,
  };

  InitResult init(IEventQueue *events, int argc, char **argv);
  void run();
  void mainLoop();
  void saveLogLevel(const QString &logLevel) const;
  void setElevate(bool elevate);
  void setCommand(const QString &command);
  void applyWatchdogCommand() const;
  void clearWatchdogCommand();
  std::string logFilename();

  static DaemonApp &instance()
  {
    static DaemonApp instance; // NOSONAR - Meyers' Singleton
    return instance;
  }

private:
  explicit DaemonApp();
  ~DaemonApp() override;

  void daemonize();
  void handleError(const char *message);
  void handleIpcMessage(const Event &e, void *);

#if SYSAPI_WIN32
  std::unique_ptr<MSWindowsWatchdog> m_watchdog;
#endif

private:
  IEventQueue *m_events = nullptr;
  FileLogOutputter *m_fileLogOutputter = nullptr;
  deskflow::core::ipc::DaemonIpcServer *m_ipcServer = nullptr;
  std::string m_command = "";
  bool m_elevate = false;
  bool m_foreground = false;
};
