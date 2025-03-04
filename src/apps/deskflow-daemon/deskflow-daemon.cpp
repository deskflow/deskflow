/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/Log.h"
#include "common/constants.h"
#include "deskflow/DaemonApp.h"
#include "deskflow/ipc/DaemonIpcServer.h"

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <QCoreApplication>
#include <QThread>

using namespace deskflow::core;

void handleError(const char *message);

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  // Save window instance for later use, e.g. `GetModuleFileName` which is used when installing the daemon.
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  auto &daemon = DaemonApp::instance();
  DaemonApp::InitResult initResult;
  try {
    initResult = daemon.init(&events, argc, argv);
  } catch (std::exception &e) {
    handleError(e.what());
    return kExitFailed;
  } catch (...) {
    handleError("Unrecognized error.");
    return kExitFailed;
  }

  // Important: Log the app name and version number to the log file after the daemon app init
  // because the file log outputter is created there. Logging before would only log to stdout
  // which is not useful for troubleshooting Windows services.
  // It's important to write the version number to the log file so we can be certain the old daemon
  // was uninstalled, since sometimes Windows services can get stuck and fail to be removed.
  // TODO: move the version number logic to a shared function for all Qt apps.
  auto versionString = QString(kVersion);
  if (versionString.endsWith(QStringLiteral(".0"))) {
    versionString.chop(2);
  } else {
    versionString.append(QStringLiteral(" (%1)").arg(kVersionGitSha));
  }
  LOG_PRINT("%s Daemon v%s", kAppName, versionString.toStdString().c_str());

  // Default log level to system setting (found in Registry).
  if (std::string logLevel = ARCH->setting("LogLevel"); logLevel != "") {
    CLOG->setFilter(logLevel.c_str());
    LOG_DEBUG("log level: %s", logLevel.c_str());
  }

  switch (initResult) {
    using enum DaemonApp::InitResult;

  case StartDaemon: {
    LOG_INFO("starting daemon");
    QCoreApplication app(argc, argv);

    try {
      daemon.initForRun();
    } catch (std::exception &e) {
      handleError(e.what());
      return kExitFailed;
    } catch (...) {
      handleError("Unrecognized error.");
      return kExitFailed;
    }

    QThread daemonThread;
    daemon.moveToThread(&daemonThread);

    QObject::connect(&daemonThread, &QThread::started, [&daemon, &daemonThread]() {
      LOG_DEBUG("daemon thread started");
      daemon.run();
      daemonThread.quit();
      LOG_DEBUG("daemon thread finished");
    });
    QObject::connect(&daemonThread, &QThread::finished, &app, &QCoreApplication::quit);

    ipc::DaemonIpcServer ipcServer(&app, QString::fromStdString(daemon.logFilename()));

    // Use direct connection as the daemon app is on it's own thread, and so is on a different event loop.
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::logLevelChanged, &daemon, &DaemonApp::saveLogLevel, //
        Qt::DirectConnection
    );
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::elevateModeChanged, &daemon, &DaemonApp::setElevate, //
        Qt::DirectConnection
    );
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::commandChanged, &daemon, &DaemonApp::setCommand, //
        Qt::DirectConnection
    );
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::startProcessRequested, &daemon, &DaemonApp::applyWatchdogCommand, //
        Qt::DirectConnection
    );
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::stopProcessRequested, &daemon, &DaemonApp::clearWatchdogCommand, //
        Qt::DirectConnection
    );
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::clearSettingsRequested, &daemon, &DaemonApp::clearSettings, //
        Qt::DirectConnection
    );

    daemonThread.start();
    const auto exitCode = QCoreApplication::exec();
    daemonThread.wait();

    LOG_DEBUG("daemon exited, code: %d", exitCode);
    return exitCode;
  }

  case FatalError:
    LOG_ERR("fatal error during daemon init");
    return kExitFailed;

  case Installed:
  case Uninstalled:
  case ShowHelp:
    LOG_DEBUG("app exiting (non-daemon mode)");
    return kExitSuccess;

  case ArgsError:
    LOG_ERR("bad arguments");
    return kExitArgs;
  }

  LOG_ERR("unknown daemon init result");
  return kExitFailed;
}

#if SYSAPI_WIN32

// Win32 subsystem entry point (simply forwards to main).
// We need this because using regular main under the Win32 subsystem results in empty args.
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  return main(__argc, __argv);
}

#endif

void handleError(const char *message)
{
  // Always print error to stdout in case run as CLI program.
  LOG_ERR("%s", message);

#if SYSAPI_WIN32
  // Show a message box for when run from MSI in Win32 subsystem.
  MessageBoxA(nullptr, message, "Deskflow daemon error", MB_OK | MB_ICONERROR);
#endif
}
