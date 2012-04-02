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
#include "CScreen.h"
#include "CMSWindowsScreen.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "CSocketMultiplexer.h"
#include "CEventQueue.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

CDaemonApp* CDaemonApp::instance = NULL;

CDaemonApp::CDaemonApp()
{
	instance = this;
}

CDaemonApp::~CDaemonApp()
{
}

int
mainLoopStatic()
{
	CDaemonApp::instance->mainLoop();
	return kExitSuccess;
}

int
unixMainLoopStatic(int, const char**)
{
	return mainLoopStatic();
}

int
winMainLoopStatic(int, const char**)
{
	return CArchMiscWindows::runDaemon(mainLoopStatic);
}

int
CDaemonApp::run(int argc, char** argv)
{
	try {

#if SYSAPI_WIN32
		// TODO: can we get rid of this? what's it used for?
		CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

		CArch arch;

		// if no args, daemonize.
		if (argc == 1) {
#if SYSAPI_WIN32
			ARCH->daemonize("Synergy", winMainLoopStatic);
#else SYSAPI_UNIX
			ARCH->daemonize("Synergy", unixMainLoopStatic);
#endif
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
CDaemonApp::mainLoop()
{
	DAEMON_RUNNING(true);

	CEventQueue eventQueue;

#if SYSAPI_WIN32
	// HACK: create a dummy screen, which can handle system events 
	// (such as a stop request from the service controller).
	CMSWindowsScreen::init(CArchMiscWindows::instanceWin32());
	CScreen dummyScreen(new CMSWindowsScreen(false, true, false));
#endif

	CEvent event;
	EVENTQUEUE->getEvent(event);
	while (event.getType() != CEvent::kQuit) {
		EVENTQUEUE->dispatchEvent(event);
		CEvent::deleteData(event);
		EVENTQUEUE->getEvent(event);
	}

	DAEMON_RUNNING(false);
}
