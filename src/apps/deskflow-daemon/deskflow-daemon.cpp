/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DaemonApp.h"

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

int main(int argc, char **argv)
{
#if SYSAPI_WIN32
  // win32 instance needed for threading, etc.
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  Arch arch;
  arch.init();

  Log log;
  EventQueue events;

  DaemonApp app(&events, __argc, __argv);
  return DaemonApp::exec();
}

#if SYSAPI_WIN32

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  return main(__argc, __argv);
}

#endif
