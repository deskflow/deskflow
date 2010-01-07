/* * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CServerApp.h"
#include "CEvent.h"
#include "CLog.h"
#include "CArch.h"
#include "Version.h"
#include "CThread.h"
#include "XArch.h"

// platform dependent includes and app instances
#if WINAPI_MSWINDOWS
#include "CMSWindowsServerTaskBarReceiver.h"
#include "XArchWindows.h"
#include "CArchMiscWindows.h"
#include "resource.h"
#include "CArchAppUtilWindows.h"
#include "CMSWindowsScreen.h"
#elif WINAPI_XWINDOWS
#include "CXWindowsServerTaskBarReceiver.h"
#elif WINAPI_CARBON
#include "COSXServerTaskBarReceiver.h"
#else
#error Platform not supported.
#endif

static
IArchTaskBarReceiver*
createTaskBarReceiver(const CBufferedLogOutputter* logBuffer)
{
#if WINAPI_MSWINDOWS

	CArchMiscWindows::setIcons(
		(HICON)LoadImage(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_SYNERGY),
		IMAGE_ICON,
		32, 32, LR_SHARED),
		(HICON)LoadImage(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_SYNERGY),
		IMAGE_ICON,
		16, 16, LR_SHARED));

	return new CMSWindowsServerTaskBarReceiver(
		CMSWindowsScreen::getInstance(), logBuffer);
#elif WINAPI_XWINDOWS
	return new CXWindowsServerTaskBarReceiver(logBuffer);
#elif WINAPI_CARBON
	return new COSXServerTaskBarReceiver(logBuffer);
#endif
}

int
main(int argc, char** argv) 
{
	CServerApp app;

#if SYSAPI_WIN32

	app.m_daemonName = "Synergy+ Server";
	app.m_daemonInfo = "Shares this computers mouse and keyboard with other computers.";

	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));

#elif SYSAPI_UNIX

	app.m_daemonName = "synergys";

#endif

	return app.run(argc, argv, createTaskBarReceiver);
}
