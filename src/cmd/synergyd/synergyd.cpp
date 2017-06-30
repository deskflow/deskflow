/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "synergy/DaemonApp.h"

#include <iostream>

#ifdef SYSAPI_UNIX

int
main (int argc, char** argv) {
    DaemonApp app;
    return app.run (argc, argv);
}

#elif SYSAPI_WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI
WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
    DaemonApp app;
    return app.run (__argc, __argv);
}

#endif
