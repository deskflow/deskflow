/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DaemonApp.h"

#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "deskflow/App.h"
#include "deskflow/ipc/DaemonIpcServer.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h" // IWYU pragma: keep
#include "deskflow/Screen.h"
#include "platform/MSWindowsDebugOutputter.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsWatchdog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <filesystem>
#include <iostream>
#include <string>

using namespace deskflow::core;

void showHelp(int argc, char **argv) // NOSONAR - CLI args
{
  const auto binName = argc > 0 ? std::filesystem::path(argv[0]).filename().string() : kDaemonBinName;
  std::cout << "Usage: " << binName << " [-f|--foreground] [--install] [--uninstall]" << std::endl;
}

DaemonApp::DaemonApp(IEventQueue &events) : m_events(events)
{
  // do nothing
}

DaemonApp::~DaemonApp() = default;

void DaemonApp::saveLogLevel(const QString &logLevel) const
{
  LOG_DEBUG("log level changed: %s", logLevel.toUtf8().constData());
  CLOG->setFilter(logLevel.toUtf8().constData());
  Settings::setValue(Settings::Daemon::LogLevel, logLevel);
}

void DaemonApp::setElevate(bool elevate)
{
  LOG_DEBUG("elevate value changed: %s", elevate ? "yes" : "no");
  m_elevate = elevate;
  Settings::setValue(Settings::Daemon::Elevate, m_elevate);
}

void DaemonApp::setCommand(const QString &command)
{
  LOG_DEBUG("service command updated");
  Settings::setValue(Settings::Daemon::Command, command);
  m_command = command.toStdString();
}

void DaemonApp::applyWatchdogCommand() const
{
  LOG_DEBUG("applying watchdog command");

#if SYSAPI_WIN32
  m_pWatchdog->setProcessConfig(m_command, m_elevate);
#else
  LOG_ERR("applying watchdog command not implemented on this platform");
#endif
}

void DaemonApp::clearWatchdogCommand()
{
  LOG_DEBUG("clearing watchdog command");

  // Clear the setting to prevent it from being next time the daemon starts.
  setCommand("");

#if SYSAPI_WIN32
  m_pWatchdog->setProcessConfig("", false);
#else
  LOG_ERR("clearing watchdog command not implemented on this platform");
#endif
}

void DaemonApp::clearSettings() const
{
  LOG_INFO("clearing daemon settings");
  Settings::setValue(Settings::Daemon::Command, QVariant());
  Settings::setValue(Settings::Daemon::Elevate, QVariant());
  Settings::setValue(Settings::Daemon::LogFile, QVariant());
  Settings::setValue(Settings::Daemon::LogLevel, QVariant());
}

void DaemonApp::connectIpcServer(const ipc::DaemonIpcServer *ipcServer) const
{
  // Use direct connection as this object is on it's own thread,
  // and so is on a different event loop to the main Qt loop.
  connect(ipcServer, &ipc::DaemonIpcServer::logLevelChanged, this, &DaemonApp::saveLogLevel, Qt::DirectConnection);
  connect(ipcServer, &ipc::DaemonIpcServer::elevateModeChanged, this, &DaemonApp::setElevate, Qt::DirectConnection);
  connect(ipcServer, &ipc::DaemonIpcServer::commandChanged, this, &DaemonApp::setCommand, Qt::DirectConnection);
  connect(
      ipcServer, &ipc::DaemonIpcServer::startProcessRequested, this, &DaemonApp::applyWatchdogCommand,
      Qt::DirectConnection
  );
  connect(
      ipcServer, &ipc::DaemonIpcServer::stopProcessRequested, this, &DaemonApp::clearWatchdogCommand,
      Qt::DirectConnection
  );
  connect(
      ipcServer, &ipc::DaemonIpcServer::clearSettingsRequested, this, &DaemonApp::clearSettings, Qt::DirectConnection
  );
}

void DaemonApp::run(QThread &daemonThread)
{
  LOG_NOTE("starting daemon");

  // Important: Move the daemon app to the daemon thread before creating any more Qt objects
  // owned by the daemon app, as they will be created on the daemon thread.
  moveToThread(&daemonThread);

  connect(&daemonThread, &QThread::started, this, [this, &daemonThread]() {
    LOG_DEBUG("daemon thread started");

    if (m_foreground) {
      LOG_DEBUG("running daemon in foreground");
      mainLoop();
    } else {
      LOG_DEBUG("running daemon in background (daemonizing)");
      ARCH->daemonize([this] { return daemonLoop(); });
    }

    daemonThread.quit();
    LOG_DEBUG("daemon thread finished");
  });

#if SYSAPI_WIN32
  m_pWatchdog = std::make_unique<MSWindowsWatchdog>(m_foreground, *m_pFileLogOutputter);

  auto command = Settings::value(Settings::Daemon::Command).toString().toStdString();
  bool elevate = Settings::value(Settings::Daemon::Elevate).toBool();
  if (!command.empty()) {
    LOG_DEBUG("using last known command: %s", command.c_str());
    m_pWatchdog->setProcessConfig(command, elevate);
  }
#endif

  LOG_DEBUG("starting daemon thread");
  daemonThread.start();
}

int DaemonApp::daemonLoop()
{
#if SYSAPI_WIN32
  // Runs the daemon through the Windows service controller, which controls the program lifecycle.
  return ArchMiscWindows::runDaemon([this]() { return mainLoop(); });
#elif SYSAPI_UNIX
  return mainLoop();
#endif
}

int DaemonApp::mainLoop()
{
#if SYSAPI_WIN32
  if (m_pWatchdog == nullptr) {
    LOG_ERR("watchdog not initialized");
    return s_exitFailed;
  }
#endif

  DAEMON_RUNNING(true);

  try {
#if SYSAPI_WIN32
    // Install the platform event queue to handle service stop events.
    // This must be done on the same thread as the event loop, otherwise the service stop
    // request will not add the quit event to the event queue, and the service won't stop.
    m_events.adoptBuffer(new MSWindowsEventQueueBuffer(&m_events));

    LOG_DEBUG("starting watchdog threads");
    m_pWatchdog->startAsync();
#endif

    LOG_INFO("daemon is running");
    m_events.loop();
  } catch (std::exception &e) { // NOSONAR - Catching all exceptions
    LOG_CRIT("daemon error: %s", e.what());
  } catch (...) { // NOSONAR - Catching remaining exceptions
    LOG_CRIT("daemon unknown error");
  }

  LOG_INFO("daemon is stopping");

#if SYSAPI_WIN32
  try {
    LOG_DEBUG("stopping process watchdog");
    m_pWatchdog->stop();
  } catch (std::exception &e) { // NOSONAR - Catching all exceptions
    LOG_CRIT("daemon stop watchdog error: %s", e.what());
  } catch (...) { // NOSONAR - Catching remaining exceptions
    LOG_CRIT("daemon stop watchdog unknown error");
  }
#endif

  DAEMON_RUNNING(false);
  return s_exitSuccess;
}

QString DaemonApp::logFilename()
{
  return Settings::value(Settings::Daemon::LogFile).toString();
}

void DaemonApp::setForeground()
{
  m_foreground = true;
  showConsole();
}

void DaemonApp::initLogging()
{
#if SYSAPI_WIN32
  if (!m_foreground) {
    // Only use MS debug outputter when the process is daemonized, since stdout won't be accessible
    // in that case, but is accessible when running in the foreground.
    CLOG->insert(new MSWindowsDebugOutputter()); // NOSONAR - Adopted by `Log`
  }
#endif

  m_pFileLogOutputter = new FileLogOutputter(qPrintable(logFilename())); // NOSONAR - Adopted by `Log`
  CLOG->insert(m_pFileLogOutputter);
}

void DaemonApp::showConsole()
{
#if SYSAPI_WIN32
  // The daemon bin is compiled using the Win32 subsystem which works best for Windows services,
  // so when running as a foreground process we need to allocate a console (or we won't see output).
  // It is important to do this inside the arg check loop so that we can attach console ahead
  // of log output generated by handling other args.
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
#endif
}
