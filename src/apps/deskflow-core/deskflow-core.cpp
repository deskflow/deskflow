/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreArgParser.h"

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/Constants.h"
#include "common/ExitCodes.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ServerApp.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#include <QCoreApplication>
#endif

#include <QFileInfo>
#include <QSharedMemory>
#include <QTextStream>

void showHelp(const CoreArgParser &parser)
{
  QTextStream(stdout) << parser.helpText();
}

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  // HACK to make sure settings gets the correct qApp path
  QCoreApplication m(argc, argv);
  m.deleteLater();

  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  Arch arch;
  arch.init();

  Log log;

  QStringList args;
  for (int i = 0; i < argc; i++)
    args.append(argv[i]);

  CoreArgParser parser(args);

  // Print any parser errors
  if (!parser.errorText().isEmpty()) {
    QTextStream(stdout) << parser.errorText() << "\n";
  }

  if (parser.help()) {
    showHelp(parser);
    return s_exitSuccess;
  }

  if (parser.version()) {
    QTextStream(stdout) << parser.versionText();
    return s_exitSuccess;
  }

  if (parser.singleInstanceOnly()) {
    // Before we check any more args we need to check for a duplicate process.
    // Create a shared memory segment with a unique key
    // This is to prevent a new instance from running if one is already running
    QSharedMemory sharedMemory(kCoreBinName);

    // Attempt to attach first and detach in order to clean up stale shm chunks
    // This can happen if the previous instance was killed or crashed
    if (sharedMemory.attach())
      sharedMemory.detach();

    // If we can create 1 byte of SHM we are the only instance
    if (!sharedMemory.create(1)) {
      LOG_WARN("an instance of deskflow core is already running");
      return s_exitDuplicate;
    }
  }

  parser.parse();

  EventQueue events;
  const auto processName = QFileInfo(argv[0]).fileName();

  if (parser.serverMode()) {
    ServerApp app(&events, processName);
    return app.run();
  } else if (parser.clientMode()) {
    ClientApp app(&events, processName);
    return app.run();
  }

  return s_exitSuccess;
}
