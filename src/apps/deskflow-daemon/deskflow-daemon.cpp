/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DaemonApp.h"

#include "base/Log.h"
#include "deskflow/ipc/DaemonIpcServer.h"

#include <QCoreApplication>
#include <QThread>

using namespace deskflow::core;

int main(int argc, char **argv)
{
  LOG((CLOG_PRINT "%s daemon (v%s)", kAppName, kVersion));

  // Avoid setting Qt ownership on the daemon app, so that we can move it to a thread.
  DaemonApp daemon;
  const auto initResult = daemon.init(argc, argv);

  switch (initResult) {
    using enum DaemonApp::InitResult;

  case StartDaemon: {
    QCoreApplication app(argc, argv);

    QThread daemonThread;
    daemon.moveToThread(&daemonThread);

    QObject::connect(&daemonThread, &QThread::started, [&daemon, &daemonThread]() {
      daemon.run();
      daemonThread.quit();
    });
    QObject::connect(&daemonThread, &QThread::finished, &app, &QCoreApplication::quit);

    ipc::DaemonIpcServer ipcServer(&app, QString::fromStdString(daemon.logFilename()));

    // Use direct connection as the daemon app is on it's own thread, and so is on a different event loop.
    QObject::connect(
        &ipcServer, &ipc::DaemonIpcServer::logLevelChanged, &daemon, &DaemonApp::setLogLevel, //
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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Win32 subsystem entry point (simply forwards to main).
// We need this because using regular main under the Win32 subsystem results in empty args.
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  return main(__argc, __argv);
}

#endif
