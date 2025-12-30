/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "common/VersionInfo.h"
#include "deskflow/DaemonApp.h"
#include "deskflow/ipc/DaemonIpcServer.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QThread>

using namespace deskflow::core;

void handleError(const char *message = "Unrecognized error.");

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  ArchMiscWindows::guardRuntimeVersion();

  // Save window instance for later use, e.g. `GetModuleFileName` which is used when installing the daemon.
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  // Daemon deliberately does not have a parent, as it will be moved to a new thread.
  DaemonApp daemon(events);

  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("%1 Daemon").arg(kAppName));

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  const auto foregroundOption = QCommandLineOption({"f", "foreground"}, "Run in the foreground (show console)");
  parser.addOption(foregroundOption);

  parser.process(app);

  if (parser.isSet(foregroundOption)) {
    daemon.setForeground();
  }

  // Depends on whether foreground option was set.
  daemon.initLogging();

  // Important: Log the app name and version number to the log file daemon app has initialized
  // logging as it creates the file logger. Logging before would only log to stdout which is not
  // useful for troubleshooting Windows services.
  // It's important to write the version number to the log file so we can be certain the old daemon
  // was uninstalled, since sometimes Windows services can get stuck and fail to be removed.
  LOG_PRINT("%s v%s", qPrintable(QCoreApplication::applicationName()), kDisplayVersion);

  // Default log level to system setting (found in Registry).
  auto logLevel = Settings::value(Settings::Daemon::LogLevel).toString().toStdString();
  if (!logLevel.empty()) {
    CLOG->setFilter(logLevel.c_str());
    LOG_DEBUG("log level: %s", logLevel.c_str());
  }

  try {

#if SYSAPI_WIN32
    // Show warning if not running as admin as daemon will behave differently.
    if (!ArchMiscWindows::isProcessElevated()) {
      LOG_WARN("not running as admin, some features may not work");
    }
#endif

    const auto ipcServer = new ipc::DaemonIpcServer(&app, qPrintable(DaemonApp::logFilename())); // NOSONAR - Qt managed
    ipcServer->listen();
    daemon.connectIpcServer(ipcServer);

    QThread daemonThread;
    QObject::connect(&daemonThread, &QThread::finished, &app, &QCoreApplication::quit);
    daemon.run(daemonThread);

    const auto exitCode = QCoreApplication::exec();
    daemonThread.wait();

    LOG_DEBUG("daemon exited, code: %d", exitCode);
    return exitCode;

  } catch (std::exception &e) {
    handleError(e.what());
    return s_exitFailed;
  } catch (...) {
    handleError();
    return s_exitFailed;
  }
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
