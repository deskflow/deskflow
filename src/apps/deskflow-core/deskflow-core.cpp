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
#endif

#include <iostream>

void showHelp()
{
  std::cout << "Usage: deskflow-core <server | client> [...options]" << std::endl;
  std::cout << "server - start as a server (deskflow-server)" << std::endl;
  std::cout << "client - start as a client (deskflow-client)" << std::endl;
  std::cout << "use deskflow-core <server|client> --help for more information." << std::endl;
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
#if SYSAPI_WIN32
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

#if SYSAPI_WIN32
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    // This is called by the daemon (running in session 0) when it needs to know the name of the
    // interactive desktop.
    // It is necessary to run a utility process because the daemon runs in session 0, which does not
    // have access to the active desktop, and so cannot query it's name.
    if (arg == "--active-desktop") {
      const auto name = ArchMiscWindows::getActiveDesktopName();
      if (name.empty()) {
        LOG((CLOG_CRIT "failed to get active desktop name"));
        return kExitFailed;
      } else {
        LOG((CLOG_PRINT "%s", name.c_str()));
        return kExitSuccess;
      }
    }
  }
#endif

  if (isServer(argc, argv)) {
    ServerApp app(&events);
    return app.run(argc, argv);
  } else if (isClient(argc, argv)) {
    ClientApp app(&events);
    return app.run(argc, argv);
  } else {
    showHelp();
    return kExitArgs;
  }
}
