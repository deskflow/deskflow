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

  LOG((CLOG_PRINT "%s daemon (v%s)", kAppName, kVersion));

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

  switch (initResult) {
    using enum DaemonApp::InitResult;

  case StartDaemon: {
    LOG_INFO("starting daemon");
    QCoreApplication app(argc, argv);

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

    daemonThread.start();
    const auto exitCode = QCoreApplication::exec();
    daemonThread.wait();

    LOG_DEBUG("daemon exited, code: %d", exitCode);
    return exitCode;
  }

  case FatalError:
    return kExitFailed;

  default:
    return kExitSuccess;
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
