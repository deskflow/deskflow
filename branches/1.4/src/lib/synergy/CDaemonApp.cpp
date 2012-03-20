/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CDaemonApp.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#include "XArchWindows.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

CDaemonApp::CDaemonApp()
{
}

CDaemonApp::~CDaemonApp()
{
}

int
CDaemonApp::run(int argc, char** argv)
{
#if SYSAPI_WIN32
	// TODO: can we get rid of this? what's it used for?
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

	CArch arch;

	try {
		// if no args, daemonize.
		if (argc == 1) {
			daemonize();
		}
		else {
			for (int i = 1; i < argc; ++i) {
				string arg(argv[i]);

#if SYSAPI_WIN32
				if (arg == "--install") {
					ARCH->installDaemon();
					continue;
				}
				else if (arg == "--uninstall") {
					ARCH->uninstallDaemon();
					continue;
				}
#endif

				cerr << "unrecognized arg: " << arg << endl;
				return kExitArgs;
			}
		}
	}
	catch (XArch& e) {
		cerr << e.what() << endl;
		return kExitFailed;
	}
	catch (std::exception& e) {
		cerr << e.what() << endl;
		return kExitFailed;
	}
	catch (...) {
		cerr << "unknown exception" << endl;
		return kExitFailed;
	}

	return kExitSuccess;
}

void
CDaemonApp::daemonize()
{
	ARCH->daemonSetting("test", "test1");
	cout << "test: " << ARCH->daemonSetting("test") << endl;
}
