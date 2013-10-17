/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#define WIN32_LEAN_AND_MEAN

#include "Windows.h"
#include "CServerApp.h"
#include "CClientApp.h"
#include "CLog.h"
#include "CArch.h"
#include "CEventQueue.h"

#if WINAPI_MSWINDOWS
#include "CMSWindowsPortableTaskBarReceiver.h"
#elif WINAPI_XWINDOWS
#include "CXWindowsPortableTaskBarReceiver.h"
#elif WINAPI_CARBON
#include "COSXPortableTaskBarReceiver.h"
#else
#error Platform not supported.
#endif

INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
#if SYSAPI_WIN32
	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif
	
	CArch arch;
	arch.init();

	CLog log;
	CEventQueue events;

	CLOG->insert(new CMesssageBoxLogOutputter());

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
		CServerApp app(&events, createTaskBarReceiver);
		return app.run(argc, argv);
	}
	else if (client) {
		CClientApp app(&events, createTaskBarReceiver);
		return app.run(argc, argv);
	}

	return 0;
}
