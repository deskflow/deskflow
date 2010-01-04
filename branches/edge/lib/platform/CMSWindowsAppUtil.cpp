/*
* synergy -- mouse and keyboard sharing utility
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

#include "CMSWindowsAppUtil.h"
#include "Version.h"
#include "CLog.h"
#include "XArchWindows.h"
#include "CArchMiscWindows.h"
#include "CApp.h"

#include <sstream>
#include <iostream>
#include <conio.h>

CMSWindowsAppUtil::CMSWindowsAppUtil()
{
}

CMSWindowsAppUtil::~CMSWindowsAppUtil()
{
}

void
CMSWindowsAppUtil::adoptApp(CApp* app)
{
	app->m_bye = &exitPause;
	CAppUtil::adoptApp(app);
}

CString
CMSWindowsAppUtil::getServiceArgs() const
{
	std::stringstream argBuf;
	for (int i = 1; i < __argc; i++) {
		const char* arg = __argv[i];

		// ignore service setup args
		if (_stricmp(arg, "--service") == 0) {
			// ignore and skip the next arg also (service action)
			i++;
		}
		else {
			argBuf << " " << __argv[i];
		}
	}
	return argBuf.str();
}

void
CMSWindowsAppUtil::handleServiceArg(const char* serviceAction)
{
	if (_stricmp(serviceAction, "install") == 0) {
		installService();
	}
	else if (_stricmp(serviceAction, "uninstall") == 0) {
		uninstallService();
	}
	else if (_stricmp(serviceAction, "start") == 0) {
		startService();
	}
	else if (_stricmp(serviceAction, "stop") == 0) {
		stopService();
	}
	else {
		LOG((CLOG_ERR "unknown service action: %s", serviceAction));
		app().m_bye(kExitArgs);
	}
	app().m_bye(kExitSuccess);
}

void
CMSWindowsAppUtil::installService()
{
	CString args = getServiceArgs();

	// get the path of this program
	char thisPath[MAX_PATH];
	GetModuleFileName(m_instance, thisPath, MAX_PATH);

	ARCH->installDaemon(
		app().m_daemonName.c_str(), app().m_daemonInfo.c_str(), 
		thisPath, args.c_str(), NULL, true);

	LOG((CLOG_INFO "service '%s' installed with args: %s",
		app().m_daemonName.c_str(), args != "" ? args.c_str() : "none" ));
}

void
CMSWindowsAppUtil::uninstallService()
{
	ARCH->uninstallDaemon(app().m_daemonName.c_str(), true);
	LOG((CLOG_INFO "service '%s' uninstalled", app().m_daemonName.c_str()));
}

void
CMSWindowsAppUtil::startService()
{
	// open service manager
	SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (mgr == NULL) {
		throw XArchEvalWindows();
	}

	// open the service
	SC_HANDLE service = OpenService(
		mgr, app().m_daemonName.c_str(), SERVICE_START);

	if (service == NULL) {
		CloseServiceHandle(mgr);
		throw XArchEvalWindows();
	}

	// start the service
	if (StartService(service, 0, NULL)) {
		LOG((CLOG_INFO "service '%s' started", app().m_daemonName.c_str()));
	}
	else {
		throw XArchEvalWindows();
	}
}

void
CMSWindowsAppUtil::stopService()
{
	// open service manager
	SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (mgr == NULL) {
		throw XArchEvalWindows();
	}

	// open the service
	SC_HANDLE service = OpenService(
		mgr, app().m_daemonName.c_str(),
		SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (service == NULL) {
		CloseServiceHandle(mgr);
		throw XArchEvalWindows();
	}

	// ask the service to stop, asynchronously
	SERVICE_STATUS ss;
	if (!ControlService(service, SERVICE_CONTROL_STOP, &ss)) {
		DWORD dwErrCode = GetLastError(); 
		if (dwErrCode != ERROR_SERVICE_NOT_ACTIVE) {
			LOG((CLOG_ERR "cannot stop service '%s'", app().m_daemonName.c_str()));
			throw XArchEvalWindows();
		}
	}

	LOG((CLOG_INFO "service '%s' stopping asyncronously", app().m_daemonName.c_str()));
}

void
exitPause(int code)
{
	CString name;
	CArchMiscWindows::getParentProcessName(name);

	// if the user did not launch from the command prompt (i.e. it was launched
	// by double clicking, or through a debugger), allow user to read any error
	// messages (instead of the window closing automatically).
	if (name != "cmd.exe") {
		std::cout << std::endl << "Press any key to exit...";
		int c = _getch();
	}

	exit(code);
}
