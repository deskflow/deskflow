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

#include "CEvent.h"
#include "CLog.h"
#include "CArch.h"
#include "Version.h"
#include "CThread.h"

// platform dependant includes and app instances
#if WINAPI_MSWINDOWS

#include "CMSWindowsServerApp.h"
#include "CMSWindowsAppUtil.h"
#include "CMSWindowsServerTaskBarReceiver.h"
#include "XArchWindows.h"
#include "CMSWindowsScreen.h"
#include "CArchMiscWindows.h"
#include "resource.h"

CServerApp* CServerApp::s_instance = new CMSWindowsServerApp();
#define APP ((CMSWindowsServerApp*)CServerApp::s_instance)

#elif WINAPI_XWINDOWS

#include "CXWindowsServerApp.h"
#include "CXWindowsAppUtil.h"
#include "CXWindowsServerTaskBarReceiver.h"

CServerApp* CServerApp::s_instance = new CXWindowsServerApp();
#define APP ((CXWindowsServerApp*)CServerApp::s_instance)

#elif WINAPI_CARBON
#include "COSXServerApp.h"
#include "COSXAppUtil.h"
#include "COSXServerTaskBarReceiver.h"

CServerApp* CServerApp::s_instance = new COSXServerApp();
#define APP ((COSXServerApp*)CServerApp::s_instance)

#else
#error Platform not supported.
#endif

// platform dependent name of a daemon
#if SYSAPI_WIN32
#define DAEMON_NAME "Synergy+ Server"
#define DAEMON_INFO "Shares this computers mouse and keyboard with other computers."
#elif SYSAPI_UNIX
#define DAEMON_NAME "synergys"
#endif

CEvent::Type
getReloadConfigEvent()
{
	return APP->getReloadConfigEvent();
}

CEvent::Type
getForceReconnectEvent()
{
	return APP->getForceReconnectEvent();
}

CEvent::Type
getResetServerEvent()
{
	return APP->getResetServerEvent();
}

static
CServerTaskBarReceiver*
createTaskBarReceiver(const CBufferedLogOutputter* logBuffer)
{
#if WINAPI_MSWINDOWS
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
#if SYSAPI_WIN32

	APP->m_daemonInfo = DAEMON_INFO;

	// get window instance for tray icon, etc
	HINSTANCE instance = GetModuleHandle(NULL);
	APP->util().m_instance = instance;

	// creates arch singleton, with window instance
	CArch arch(instance);

#elif SYSAPI_UNIX

	// creates arch singleton
	CArch arch;

#endif

	CLOG;
	APP->m_daemonName = DAEMON_NAME;
	int result;

	try {

#if SYSAPI_WIN32
		if (!instance) {
			throw XArchDaemon(new XArchEvalWindows());
		}

		CArchMiscWindows::setIcons(
			(HICON)LoadImage(instance,
			MAKEINTRESOURCE(IDI_SYNERGY),
			IMAGE_ICON,
			32, 32, LR_SHARED),
			(HICON)LoadImage(instance,
			MAKEINTRESOURCE(IDI_SYNERGY),
			IMAGE_ICON,
			16, 16, LR_SHARED));

		CMSWindowsScreen::init(instance);
		CThread::getCurrentThread().setPriority(-14);
#endif

		result = APP->run(argc, argv, createTaskBarReceiver);
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "Uncaught exception: %s\n", e.what()));
		result = kExitFailed;
	}
	catch (XArch& e) {
		LOG((CLOG_CRIT "Initialization failed: %s" BYE, e.what().c_str()));
		result = kExitFailed;
	}
	catch (std::exception& e) {
		LOG((CLOG_CRIT "Uncaught exception: %s\n", e.what()));
		result = kExitFailed;
	}
	catch (...) {
		LOG((CLOG_CRIT "Uncaught exception: <unknown exception>\n"));
		result = kExitFailed;
	}

	delete CLOG;
	APP->m_bye(result);
}
