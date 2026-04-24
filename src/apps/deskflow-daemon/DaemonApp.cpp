/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonApp.h"

#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "deskflow/ipc/DaemonIpcServer.h"

#if defined(Q_OS_WIN)
#include "arch/win32/ArchDaemonWindows.h"
#include "deskflow/Screen.h"
#include "platform/MSWindowsDebugOutputter.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsWatchdog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

using namespace deskflow::core;

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

void DaemonApp::setConfigFile(const QString &configFile)
{
  LOG_DEBUG("config file updated: %s", configFile.toUtf8().constData());
  m_configFile = configFile;
  Settings::setValue(Settings::Daemon::ConfigFile, configFile);
}

void DaemonApp::applyWatchdogCommand() const
{
#if defined(Q_OS_WIN)
  if (m_configFile.isEmpty()) {
    LOG_ERR("cannot apply watchdog command: no config file set");
    return;
  }

  // QFileInfo::exists on a UNC path triggers SMB auth from this SYSTEM-context
  // process, leaking the machine NTLM hash to whoever controls the remote host.
  // Any local user can reach this via the IPC pipe, so reject remote paths up front.
  if (m_configFile.startsWith(QStringLiteral("\\\\")) || m_configFile.startsWith(QStringLiteral("//"))) {
    LOG_ERR("cannot apply watchdog command: remote config file paths are not allowed: %s", qPrintable(m_configFile));
    return;
  }

  if (!QFileInfo::exists(m_configFile)) {
    LOG_ERR("cannot apply watchdog command: config file does not exist: %s", qPrintable(m_configFile));
    return;
  }

  QSettings config(m_configFile, QSettings::IniFormat);
  const auto coreMode = config.value(Settings::Core::CoreMode).toInt();
  const auto elevate = config.value(Settings::Daemon::Elevate, !Settings::isPortableMode()).toBool();

  QString modeArg;
  if (coreMode == Settings::CoreMode::Server) {
    modeArg = QStringLiteral("server");
  } else if (coreMode == Settings::CoreMode::Client) {
    modeArg = QStringLiteral("client");
  } else {
    LOG_ERR("cannot apply watchdog command: invalid core mode in config: %d", coreMode);
    return;
  }

  const auto corePath = QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), kCoreBinName);
  const auto command = QStringLiteral("\"%1\" %2 --settings \"%3\"").arg(corePath, modeArg, m_configFile).toStdString();

  LOG_DEBUG("applying watchdog command (elevate: %s)", elevate ? "yes" : "no");
  m_pWatchdog->setProcessConfig(command, elevate);
#else
  LOG_ERR("applying watchdog command not implemented on this platform");
#endif
}

void DaemonApp::clearWatchdogCommand()
{
  LOG_DEBUG("clearing watchdog command");

  // Clear the persisted config path so the daemon does not auto-start the core on next boot.
  m_configFile.clear();
  Settings::setValue(Settings::Daemon::ConfigFile);

#if defined(Q_OS_WIN)
  m_pWatchdog->setProcessConfig("", false);
#else
  LOG_ERR("clearing watchdog command not implemented on this platform");
#endif
}

void DaemonApp::clearSettings()
{
  LOG_INFO("clearing daemon settings");
  m_configFile.clear();
  Settings::setValue(Settings::Daemon::ConfigFile);
  Settings::setValue(Settings::Daemon::LogFile);
  Settings::setValue(Settings::Daemon::LogLevel);
}

void DaemonApp::connectIpcServer(const ipc::DaemonIpcServer *ipcServer) const
{
  // Use direct connection as this object is on it's own thread,
  // and so is on a different event loop to the main Qt loop.
  connect(ipcServer, &ipc::DaemonIpcServer::logLevelChanged, this, &DaemonApp::saveLogLevel, Qt::DirectConnection);
  connect(ipcServer, &ipc::DaemonIpcServer::configFileChanged, this, &DaemonApp::setConfigFile, Qt::DirectConnection);
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

#if defined(Q_OS_WIN)
  m_pWatchdog = std::make_unique<MSWindowsWatchdog>(m_foreground, *m_pFileLogOutputter);

  if (const auto persistedConfig = Settings::value(Settings::Daemon::ConfigFile).toString();
      !persistedConfig.isEmpty()) {
    LOG_DEBUG("using last known config file: %s", persistedConfig.toUtf8().constData());
    m_configFile = persistedConfig;
    applyWatchdogCommand();
  }
#endif

  LOG_DEBUG("starting daemon thread");
  daemonThread.start();
}

int DaemonApp::daemonLoop()
{
#if defined(Q_OS_WIN)
  // Runs the daemon through the Windows service controller, which controls the program lifecycle.
  return ArchDaemonWindows::runDaemon([this]() { return mainLoop(); });
#else
  return mainLoop();
#endif
}

int DaemonApp::mainLoop()
{
#if defined(Q_OS_WIN)
  if (m_pWatchdog == nullptr) {
    LOG_ERR("watchdog not initialized");
    return s_exitFailed;
  }
  ArchDaemonWindows::daemonRunning(true);
#endif

  try {
#if defined(Q_OS_WIN)
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

#if defined(Q_OS_WIN)
  try {
    LOG_DEBUG("stopping process watchdog");
    m_pWatchdog->stop();
  } catch (std::exception &e) { // NOSONAR - Catching all exceptions
    LOG_CRIT("daemon stop watchdog error: %s", e.what());
  } catch (...) { // NOSONAR - Catching remaining exceptions
    LOG_CRIT("daemon stop watchdog unknown error");
  }
  ArchDaemonWindows::daemonRunning(false);
#endif

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
#if defined(Q_OS_WIN)
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
#if defined(Q_OS_WIN)
  // The daemon bin is compiled using the Win32 subsystem which works best for Windows services,
  // so when running as a foreground process we need to allocate a console (or we won't see output).
  // It is important to do this inside the arg check loop so that we can attach console ahead
  // of log output generated by handling other args.
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
#endif
}
