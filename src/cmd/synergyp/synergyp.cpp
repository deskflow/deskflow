/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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

#include "synergy/ServerApp.h"
#include "synergy/ClientApp.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/EventQueue.h"

#if WINAPI_MSWINDOWS
#include "MSWindowsPortableTaskBarReceiver.h"
#elif WINAPI_XWINDOWS
#include "XWindowsPortableTaskBarReceiver.h"
#elif WINAPI_CARBON
#include "OSXPortableTaskBarReceiver.h"
#else
#error Platform not supported.
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
#if SYSAPI_WIN32
	// record window instance for tray icon, etc
	ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif
	
	Arch arch;
	arch.init();

	Log log;
	EventQueue events;

	CLOG->insert(new MesssageBoxLogOutputter());

	int argc = __argc;
	char** argv = __argv;

	bool server = false, client = false;
	for (int i = 0; i < argc; i++) {
		if (std::string(argv[i]) == "--server") {
			server = true;
		}
		else if (std::string(argv[i]) == "--client") {
			client = true;
		}
	}

	if (!server && !client) {
		MessageBox(NULL,
			"Either the --server argument or the --client argument must be provided.",
			"Server or client?", MB_OK);
        return 1;
	}
	
	if (argc <= 2) {
		MessageBox(NULL,
			"No additional arguments were provided. Append the --help argument for help.\n\n"
			"Hint: Create a shortcut and append the \"Target\" field with the arguments.",
			"No additional arguments", MB_OK);
        return 1;
	}
	
	if (server) {
		ServerApp app(&events, createTaskBarReceiver);
		return app.run(argc, argv);
	}
	else if (client) {
		ClientApp app(&events, createTaskBarReceiver);
		return app.run(argc, argv);
	}

	return 0;
}
