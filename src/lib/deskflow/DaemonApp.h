/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"

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
  enum class InitResult
  {
    Installed,
    Uninstalled,
    StartDaemon,
    ShowHelp,
    ArgsError,
    FatalError,
  };

  explicit DaemonApp();
  ~DaemonApp();
  InitResult init(int argc, char **argv);
  void run();
  void mainLoop(bool foreground = false);
  void applyWatchdogCommand();
  void clearWatchdogCommand();

  // Setters
  void setLogLevel(const QString &logLevel);
  void setElevate(bool elevate);
  void setCommand(const QString &command);

  // Getters
  std::string logFilename();

  static DaemonApp *instance()
  {
    return s_instance;
  }

private:
  void daemonize();
  void handleError(const char *message);
  void handleIpcMessage(const Event &e, void *);

private:
  static DaemonApp *s_instance;

#if SYSAPI_WIN32
  std::unique_ptr<MSWindowsWatchdog> m_watchdog;
#endif

private:
  std::unique_ptr<IEventQueue> m_events;
  std::unique_ptr<FileLogOutputter> m_fileLogOutputter;
  std::string m_command = "";
  bool m_elevate = false;
  bool m_foreground = false;
};
