/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2022 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ServerApp.h"
#include <iostream>

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

void showHelp()
{
  std::cout << "Usage: " CORE_BINARY_NAME " <server | client> [...options]" << std::endl;
  std::cout << "server - start as a server ( deskflow-server)" << std::endl;
  std::cout << "client - start as a client (" << CLIENT_BINARY_NAME << ")" << std::endl;
  std::cout << "use " CORE_BINARY_NAME " <server|client> --help for more information." << std::endl;
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

  if (isServer(argc, argv)) {
    ServerApp app(&events, nullptr);
    return app.run(argc, argv);
  } else if (isClient(argc, argv)) {
    ClientApp app(&events, nullptr);
    return app.run(argc, argv);
  } else {
    showHelp();
  }

  return 0;
}
