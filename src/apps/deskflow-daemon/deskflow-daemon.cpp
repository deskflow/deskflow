/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
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

  QCoreApplication app(argc, argv);

  const auto pDaemon = &DaemonApp::instance();
  const auto initResult = pDaemon->init(&events, argc, argv);

  switch (initResult) {
    using enum DaemonApp::InitResult;

  case StartDaemon: {
    using namespace deskflow::core;

    QCoreApplication app(argc, argv);

    // Thread must be heap-allocated for deferred deletion on thread exit.
    // Avoid setting Qt ownership to prevent premature deletion (thread may run longer than Qt loop).
    auto *pDaemonThread = new QThread(); // NOSONAR
    pDaemon->moveToThread(pDaemonThread);

    QObject::connect(pDaemonThread, &QThread::started, pDaemon, &DaemonApp::run);
    QObject::connect(pDaemonThread, &QThread::finished, pDaemon, &QObject::deleteLater);
    QObject::connect(pDaemonThread, &QThread::finished, pDaemonThread, &QThread::deleteLater);
    QObject::connect(pDaemonThread, &QThread::finished, QCoreApplication::instance(), &QCoreApplication::quit);

    // The daemon app is on it's own thread which doesn't have a Qt event loop, so we need to use direct connection.
    auto *ipcServer = new ipc::DaemonIpcServer(&app); // NOSONAR
    QObject::connect(
        ipcServer, &ipc::DaemonIpcServer::logLevelChanged, pDaemon, &DaemonApp::saveLogLevel, //
        Qt::DirectConnection
    );
    QObject::connect(
        ipcServer, &ipc::DaemonIpcServer::elevateModeChanged, pDaemon, &DaemonApp::setElevate, //
        Qt::DirectConnection
    );
    QObject::connect(
        ipcServer, &ipc::DaemonIpcServer::commandChanged, pDaemon, &DaemonApp::setCommand, //
        Qt::DirectConnection
    );
    QObject::connect(
        ipcServer, &ipc::DaemonIpcServer::startProcessRequested, pDaemon, &DaemonApp::applyWatchdogCommand, //
        Qt::DirectConnection
    );
    QObject::connect(
        ipcServer, &ipc::DaemonIpcServer::stopProcessRequested, pDaemon, &DaemonApp::clearWatchdogCommand, //
        Qt::DirectConnection
    );

    pDaemonThread->start();
    return QCoreApplication::exec();
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
