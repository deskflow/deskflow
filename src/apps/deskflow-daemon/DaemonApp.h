/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

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
  explicit DaemonApp(IEventQueue &events);
  ~DaemonApp() override;

  void run(QThread &daemonThread);
  void setForeground();
  void initLogging();
  void connectIpcServer(const deskflow::core::ipc::DaemonIpcServer *ipcServer) const;

  static QString logFilename();

private:
  void daemonize();
  void handleError(const char *message);
  int mainLoop();
  int daemonLoop();
  void saveLogLevel(const QString &logLevel) const;
  void setElevate(bool elevate);
  void setCommand(const QString &command);
  void applyWatchdogCommand() const;
  void clearWatchdogCommand();
  void clearSettings() const;

  static void showConsole();

#if SYSAPI_WIN32
  std::unique_ptr<MSWindowsWatchdog> m_pWatchdog;
#endif

  IEventQueue &m_events;
  FileLogOutputter *m_pFileLogOutputter = nullptr;
  deskflow::core::ipc::DaemonIpcServer *m_ipcServer = nullptr;
  std::string m_command = "";
  bool m_elevate = false;
  bool m_foreground = false;
};
