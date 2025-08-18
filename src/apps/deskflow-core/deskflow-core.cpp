/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ServerApp.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#include <QCoreApplication>
#endif

#include <QSharedMemory>
#include <iostream>

void showHelp()
{
  std::cout << "Usage: deskflow-core <server | client> [...options]" << std::endl;
  std::cout << "server - start as a server (deskflow-server)" << std::endl;
  std::cout << "client - start as a client (deskflow-client)" << std::endl;

  ServerApp sApp(nullptr);
  sApp.help();

  ClientApp cApp(nullptr);
  cApp.help();
}

bool isHelp(int argc, char **argv)
{
  for (int i = 0; i < argc; ++i) {
    if (argv[i] == std::string("--help") || argv[i] == std::string("-h"))
      return true;
  }
  return false;
}

bool isServer(int argc, char **argv)
{
  return (argc > 1 && argv[1] == std::string("server"));
}

bool isClient(int argc, char **argv)
{
  return (argc > 1 && argv[1] == std::string("client"));
}

int main(int argc, char **argv)
{
  Arch arch;
  arch.init();

  Log log;

  if (isHelp(argc, argv)) {
    showHelp();
    return 0;
  }

  // Create a shared memory segment with a unique key
  // This is to prevent a new instance from running if one is already running
  QSharedMemory sharedMemory("deskflow-core");

  // Attempt to attach first and detach in order to clean up stale shm chunks
  // This can happen if the previous instance was killed or crashed
  if (sharedMemory.attach())
    sharedMemory.detach();

  // If we can create 1 byte of SHM we are the only instance
  if (!sharedMemory.create(1)) {
    LOG_WARN("an instance of deskflow core (server or client) is already running");
    return 0;
  }
#if SYSAPI_WIN32
  // HACK to make sure settings gets the correct qApp path
  QCoreApplication m(argc, argv);
  m.deleteLater();

  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  EventQueue events;

  if (isServer(argc, argv)) {
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
