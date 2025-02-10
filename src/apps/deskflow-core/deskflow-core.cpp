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
#include <QString>
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
