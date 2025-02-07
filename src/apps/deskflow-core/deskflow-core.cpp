/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/common.h"
#include "common/constants.h"
#include "deskflow/ClientApp.h"
#include "deskflow/DaemonApp.h"
#include "deskflow/ServerApp.h"
#include <qobject.h>

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#include <QCoreApplication>
#include <QThread>

#include <iostream>

#define TEST_DAEMON

const auto kCoreBinName = "deskflow-core";

void showHelp()
{
  std::cout << "Usage: " << kCoreBinName << " <server | client> [...options]" << std::endl;
  std::cout << "server - start core as server" << std::endl;
  std::cout << "client - start core as client" << std::endl;

#ifdef SYSAPI_WIN32
  std::cout << "daemon - start as a demon (Windows only)" << std::endl;
  std::cout << "use " << kCoreBinName << " <server|client|daemon> --help for more information." << std::endl;
#else
  std::cout << "use " << kCoreBinName << " <server|client> --help for more information." << std::endl;
#endif
}

bool isServer(int argc, char **argv)
{
  return (argc > 1 && argv[1] == std::string("server"));
}

bool isClient(int argc, char **argv)
{
  return (argc > 1 && argv[1] == std::string("client"));
}

bool isDaemon(int argc, char **argv)
{
  return (argc > 1 && argv[1] == std::string("daemon"));
}

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  if (isDaemon(argc, argv)) {
    LOG((CLOG_PRINT "%s daemon (v%s)", kAppName, kVersion));
    QCoreApplication app(argc, argv);

    // Daemon app must be on the heap, as we're moving it to a thread.
    // Deliberately do not set ownership of the daemon to the app, as this would prevent us
    // being able to move it to a thread and delete it when the thread finishes.
    auto *pDaemon = new DaemonApp(); // NOSONAR
    QObject::connect(pDaemon, &DaemonApp::mainLoopFinished, &app, &QCoreApplication::quit);
    QObject::connect(pDaemon, &DaemonApp::fatalErrorOccurred, &app, &QCoreApplication::quit);
    QObject::connect(pDaemon, &DaemonApp::serviceInstalled, &app, &QCoreApplication::quit);
    QObject::connect(pDaemon, &DaemonApp::serviceUninstalled, &app, &QCoreApplication::quit);
    pDaemon->init(argc, argv);

    // Thread must be on the heap, as the thread needs to be deleted only when the loop finishes,
    // and not when we go out of scope.
    // Deliberately do not set ownership of the thread to the daemon, as we want to delete the thread
    // when the loop finishes, and not when the daemon is deleted.
    QThread *thread = new QThread(); // NOSONAR
    pDaemon->moveToThread(thread);
    QObject::connect(thread, &QThread::started, pDaemon, &DaemonApp::run);
    QObject::connect(thread, &QThread::finished, pDaemon, &QObject::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    return QCoreApplication::exec();
  } else if (isServer(argc, argv)) {
    ServerApp app(&events);
    return app.run(argc, argv);
  } else if (isClient(argc, argv)) {
    ClientApp app(&events);
    return app.run(argc, argv);
  } else {
    showHelp();
  }

  return 0;
}
