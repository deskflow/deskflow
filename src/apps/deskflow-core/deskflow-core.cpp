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
#include "deskflow/ipc/DaemonIpcServer.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#include <QCoreApplication>
#include <QObject>
#include <QThread>

#include <filesystem>
#include <iostream>

#define TEST_DAEMON

void showHelp(int argc, char **argv)
{
  const auto binName = argc > 0 ? std::filesystem::path(argv[0]).filename().string() : "deskflow-core";
  std::cout << "Usage: " << binName << " <mode> [...options]" << std::endl;

#ifdef SYSAPI_WIN32
  std::cout << "Modes: server, client, daemon" << std::endl;
#else
  std::cout << "Modes: server, client" << std::endl;
#endif

  std::cout << "Use " << binName << " <mode> --help for mode specific help." << std::endl;
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
  using namespace deskflow::core;

#if SYSAPI_WIN32
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  if (isDaemon(argc, argv)) {
    LOG((CLOG_PRINT "%s daemon (v%s)", kAppName, kVersion));

    // Daemon and thread must be heap-allocated to allow thread migration and deferred deletion.
    // Avoid setting Qt ownership to prevent premature deletion.
    auto *pDaemon = new DaemonApp(); // NOSONAR
    const auto initResult = pDaemon->init(argc, argv);

    switch (initResult) {
      using enum DaemonApp::InitResult;

    case StartDaemon: {
      QCoreApplication app(argc, argv);

      auto *pDaemonThread = new QThread(); // NOSONAR
      pDaemon->moveToThread(pDaemonThread);

      QObject::connect(pDaemonThread, &QThread::started, pDaemon, &DaemonApp::run);
      QObject::connect(pDaemonThread, &QThread::finished, pDaemon, &QObject::deleteLater);
      QObject::connect(pDaemonThread, &QThread::finished, pDaemonThread, &QThread::deleteLater);
      QObject::connect(pDaemonThread, &QThread::finished, QCoreApplication::instance(), &QCoreApplication::quit);

      // The daemon app is on it's own thread which doesn't have a Qt event loop, so we need to use direct connection.
      auto *ipcServer = new ipc::DaemonIpcServer(&app); // NOSONAR
      QObject::connect(
          ipcServer, &ipc::DaemonIpcServer::logLevelChanged, pDaemon, &DaemonApp::setLogLevel, //
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
          ipcServer, &ipc::DaemonIpcServer::restartRequested, pDaemon, &DaemonApp::restartCoreProcess, //
          Qt::DirectConnection
      );
      pDaemonThread->start();
      return QCoreApplication::exec();
    }

    case ArgsError:
      showHelp(argc, argv);
      return kExitArgs;

    case FatalError:
      return kExitFailed;

    default:
      return kExitSuccess;
    }

  } else if (isServer(argc, argv)) {
    ServerApp app(&events);
    return app.run(argc, argv);
  } else if (isClient(argc, argv)) {
    ClientApp app(&events);
    return app.run(argc, argv);
  } else {
    showHelp(argc, argv);
    return kExitArgs;
  }

  return kExitSuccess;
}
