/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

#include <string>

#include <QObject>
#include <QThread>

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

  explicit DaemonApp(IEventQueue &events);
  ~DaemonApp() override;

  InitResult init(int argc, char **argv);
  void install() const;
  void uninstall() const;
  void run(QThread &daemonThread);
  void showConsole() const;
  void setForeground();
  void initLogging();
  void connectIpcServer(deskflow::core::ipc::DaemonIpcServer *ipcServer) const;

  static std::string logFilename();

private:
  void daemonize();
  void handleError(const char *message);
  void handleIpcMessage(const Event &e, void *);
  void mainLoop();
  void saveLogLevel(const QString &logLevel) const;
  void setElevate(bool elevate);
  void setCommand(const QString &command);
  void applyWatchdogCommand() const;
  void clearWatchdogCommand();
  void clearSettings() const;

  IEventQueue &m_events;
  FileLogOutputter *m_pFileLogOutputter = nullptr;
  std::string m_command = "";
  bool m_elevate = false;
  bool m_foreground = false;

#if SYSAPI_WIN32
  std::shared_ptr<MSWindowsWatchdog> m_pWatchdog;
#endif
};
