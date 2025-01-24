/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DaemonApp.h"

#include <iostream>

#ifdef SYSAPI_UNIX

int main(int argc, char **argv)
{
  DaemonApp app;
  return app.run(argc, argv);
}

#elif SYSAPI_WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  DaemonApp app;
  return app.run(__argc, __argv);
}

#endif
