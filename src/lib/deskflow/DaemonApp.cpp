/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DaemonApp.h"

#include "arch/XArch.h"
#include "base/Log.h"
#include "base/log_outputters.h"
#include "common/constants.h"
#include "deskflow/App.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"
#include "deskflow/Screen.h"
#include "platform/MSWindowsDebugOutputter.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsWatchdog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

using namespace std;
using namespace deskflow::core;

void showHelp(int argc, char **argv) // NOSONAR - CLI args
{
  const auto binName = argc > 0 ? std::filesystem::path(argv[0]).filename().string() : kDaemonBinName;
  std::cout << "Usage: " << binName << " [-f|--foreground] [--install] [--uninstall]" << std::endl;
}

DaemonApp::DaemonApp()
{
  m_fileLogOutputter = new FileLogOutputter(logFilename().c_str()); // NOSONAR - Adopted by `Log`
  CLOG->insert(m_fileLogOutputter);
}

DaemonApp::~DaemonApp() = default;

int daemonLoop()
{
  DaemonApp::instance().mainLoop();
  return kExitSuccess;
}

#if SYSAPI_WIN32
int daemonLoop(int, const char **)
{
  return ArchMiscWindows::runDaemon(daemonLoop);
}
#elif SYSAPI_UNIX
int daemonLoop(int, const char **)
{
  return daemonLoop();
}
#endif

void DaemonApp::run()
{
  if (m_foreground) {
    LOG_DEBUG("running daemon in foreground");
    mainLoop();
  } else {
    LOG_DEBUG("running daemon in background (daemonizing)");
    ARCH->daemonize(kAppName, daemonLoop);
  }
}

void DaemonApp::saveLogLevel(const QString &logLevel) const
{
  LOG_DEBUG("log level changed: %s", logLevel.toUtf8().constData());
  CLOG->setFilter(logLevel.toUtf8().constData());

  try {
    // saves setting for next time the daemon starts.
    ARCH->setting("LogLevel", logLevel.toStdString());
  } catch (XArch &e) {
    LOG((CLOG_ERR "failed to save log level setting: %s", e.what()));
  }
}

void DaemonApp::setElevate(bool elevate)
{
  LOG((CLOG_DEBUG "elevate value changed: %s", elevate ? "yes" : "no"));
  m_elevate = elevate;

  try {
    // saves setting for next time the daemon starts.
    ARCH->setting("Elevate", std::string(elevate ? "1" : "0"));
  } catch (XArch &e) {
    LOG((CLOG_ERR "failed to save elevate setting: %s", e.what()));
  }
}

void DaemonApp::setCommand(const QString &command)
{
  LOG_DEBUG("service command updated");
  m_command = command.toStdString();

  try {
    // saves setting for next time the daemon starts.
    ARCH->setting("Command", command.toStdString());
  } catch (XArch &e) {
    LOG((CLOG_ERR "failed to save command setting: %s", e.what()));
  }
}

void DaemonApp::applyWatchdogCommand() const
{
  LOG_DEBUG("applying watchdog command");

#if SYSAPI_WIN32
  m_watchdog->setProcessConfig(m_command, m_elevate);
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
  m_watchdog->setProcessConfig("", false);
#else
  LOG_ERR("clearing watchdog command not implemented on this platform");
#endif
}

void DaemonApp::clearSettings() const
{
  LOG_INFO("clearing daemon settings");
  ARCH->clearSettings();
}

DaemonApp::InitResult DaemonApp::init(IEventQueue *events, int argc, char **argv) // NOSONAR - CLI args
{
  using enum InitResult;

  if (events == nullptr) {
    throw XDeskflow("event queue not set");
  }

  m_events = events;

  for (int i = 1; i < argc; ++i) {
    string arg(argv[i]);

    if (arg == "-h" || arg == "--help") {
      showConsole();
      showHelp(argc, argv);
      return ShowHelp;
    } else if (arg == "-f" || arg == "--foreground") {
      m_foreground = true;
      showConsole();
    }
#if SYSAPI_WIN32
    else if (arg == "--install" || arg == "/install") {
      initDebugOutput();
      LOG((CLOG_NOTE "installing windows daemon"));
      ARCH->installDaemon();
      return Installed;
    } else if (arg == "--uninstall" || arg == "/uninstall") {
      initDebugOutput();
      LOG((CLOG_NOTE "uninstalling windows daemon"));
      try {
        ARCH->uninstallDaemon();
      } catch (XArch &e) {
        std::string message = e.what();
        if (message.find("The service has not been started") != std::string::npos) {
          // HACK: this message happens intermittently, not sure where from but
          // it's quite misleading for the user. they thing something has gone
          // horribly wrong, but it's just the service manager reporting a false
          // positive (the service has actually shut down in most cases).
          LOG_DEBUG("ignoring service start error on uninstall: %s", message.c_str());
        } else {
          throw e;
        }
      }
      return Uninstalled;
    }
#endif
    else {
      LOG_ERR("unknown argument: %s", arg.c_str());
      return ArgsError;
    }
  }

  initDebugOutput();
  return StartDaemon;
}

void DaemonApp::showConsole() const
{
#if SYSAPI_WIN32
  // The daemon bin is compiled using the Win32 subsystem which works best for Windows services,
  // so when running as a foreground process we need to allocate a console (or we won't see output).
  // It is important to do this inside the arg check loop so that we can attach console ahead
  // of log output generated by handling other args.
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
#endif
}

void DaemonApp::initDebugOutput() const
{
#if SYSAPI_WIN32
  if (!m_foreground) {
    // Only use MS debug outputter when the process is daemonized, since stdout won't be accessible
    // in that case, but is accessible when running in the foreground.
    CLOG->insert(new MSWindowsDebugOutputter()); // NOSONAR - Adopted by `Log`
  }
#endif
}

void DaemonApp::initForRun()
{
#if SYSAPI_WIN32
  LOG_DEBUG("initializing daemon for run mode");

  if (m_events == nullptr) {
    throw XDeskflow("event queue not set for pre-run init");
  }

  m_watchdog = std::make_unique<MSWindowsWatchdog>(m_foreground);
  m_watchdog->setFileLogOutputter(m_fileLogOutputter);

  // install the platform event queue to handle service stop events.
  m_events->adoptBuffer(new MSWindowsEventQueueBuffer(m_events));

  std::string command = ARCH->setting("Command");
  bool elevate = ARCH->setting("Elevate") == "1";

  if (!command.empty()) {
    LOG_DEBUG("using last known command: %s", command.c_str());
    m_watchdog->setProcessConfig(command, elevate);
  }
#endif
}

void DaemonApp::mainLoop()
{
  if (m_watchdog == nullptr) {
    LOG_CRIT("watchdog not set for main loop");
    return;
  }

  if (m_events == nullptr) {
    LOG_CRIT("event queue not set for main loop");
    return;
  }

  DAEMON_RUNNING(true);

  try {
    LOG_DEBUG("starting watchdog threads");
    m_watchdog->startAsync();

    LOG_INFO("daemon is running");
    m_events->loop();
  } catch (std::exception &e) { // NOSONAR - Catching all exceptions
    LOG_CRIT("daemon runtime error: %s", e.what());
  } catch (...) { // NOSONAR - Catching remaining exceptions
    LOG_CRIT("daemon runtime unknown error");
  }

  LOG_INFO("daemon is stopping");

#if SYSAPI_WIN32
  try {
    LOG_DEBUG("stopping process watchdog");
    m_watchdog->stop();
  } catch (std::exception &e) { // NOSONAR - Catching all exceptions
    LOG_CRIT("daemon stop watchdog error: %s", e.what());
  } catch (...) { // NOSONAR - Catching remaining exceptions
    LOG_CRIT("daemon stop watchdog unknown error");
  }
#endif

  DAEMON_RUNNING(false);
}

std::string DaemonApp::logFilename()
{
  string logFilename;
  logFilename = ARCH->setting("LogFilename");
  if (logFilename.empty()) {
    logFilename = ARCH->getLogDirectory();
    logFilename.append("/");
    logFilename.append(kDaemonLogFilename);
  }

  return logFilename;
}
