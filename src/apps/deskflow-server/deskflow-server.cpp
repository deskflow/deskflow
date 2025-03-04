/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012-2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "deskflow/ServerApp.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  // record window instance for tray icon, etc
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  // HACK: the `--active-desktop` arg actually belongs in the `deskflow-core` binary,
  // but we are placing it here in the server binary temporarily until we are ready to
  // ship the `deskflow-core` binary. we are deliberately not integrating `--active-desktop`
  // into the existing `ServerApp` arg parsing code as that would be a waste of time.
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
      }

      LOG((CLOG_PRINT "%s", name.c_str()));
      return kExitSuccess;
    }
  }
#endif

  ServerApp app(&events);
  return app.run(argc, argv);
}
