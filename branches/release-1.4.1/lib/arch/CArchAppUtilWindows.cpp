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

#include "CArchAppUtilWindows.h"
#include "Version.h"
#include "CLog.h"
#include "XArchWindows.h"
#include "CArchMiscWindows.h"
#include "CApp.h"
#include "LogOutputters.h"
#include "CMSWindowsScreen.h"
#include "XSynergy.h"
#include "IArchTaskBarReceiver.h"
#include "CMSWindowsRelauncher.h"
#include "CScreen.h"

#include <sstream>
#include <iostream>
#include <conio.h>

CArchAppUtilWindows::CArchAppUtilWindows() :
m_exitMode(kExitModeNormal)
{
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE) == FALSE)
    {
		throw XArchEvalWindows();
    }
}

CArchAppUtilWindows::~CArchAppUtilWindows()
{
}

BOOL WINAPI CArchAppUtilWindows::consoleHandler(DWORD CEvent)
{
	// HACK: it would be nice to delete the s_taskBarReceiver object, but 
	// this is best done by the CApp destructor; however i don't feel like
	// opening up that can of worms today... i need sleep.
	instance().app().s_taskBarReceiver->cleanup();

	ExitProcess(kExitTerminated);
    return TRUE;
}

bool 
CArchAppUtilWindows::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (app().isArg(i, argc, argv, NULL, "--service")) {

		const char* action = argv[++i];

		if (_stricmp(action, "install") == 0) {
			installService();
		}
		else if (_stricmp(action, "uninstall") == 0) {
			uninstallService();
		}
		else if (_stricmp(action, "start") == 0) {
			startService();
		}
		else if (_stricmp(action, "stop") == 0) {
			stopService();
		}
		else {
			LOG((CLOG_ERR "unknown service action: %s", action));
			app().m_bye(kExitArgs);
		}
		app().m_bye(kExitSuccess);
	}
	else if (app().isArg(i, argc, argv, NULL, "--debug-service-wait")) {

		app().argsBase().m_debugServiceWait = true;
	}
	else if (app().isArg(i, argc, argv, NULL, "--relaunch")) {
		app().argsBase().m_relaunchMode = true;
	}
	else {
		// option not supported here
		return false;
	}

	return true;
}

CString
CArchAppUtilWindows::getServiceArgs() const
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
CArchAppUtilWindows::installService()
{
	CString args = getServiceArgs();

	// get the path of this program
	char thisPath[MAX_PATH];
	GetModuleFileName(CArchMiscWindows::instanceWin32(), thisPath, MAX_PATH);

	ARCH->installDaemon(
		app().daemonName(), app().daemonInfo(), 
		thisPath, args.c_str(), NULL, true);

	LOG((CLOG_INFO "service '%s' installed with args: %s",
		app().daemonName(), args != "" ? args.c_str() : "none" ));
}

void
CArchAppUtilWindows::uninstallService()
{
	ARCH->uninstallDaemon(app().daemonName(), true);
	LOG((CLOG_INFO "service '%s' uninstalled", app().daemonName()));
}

void
CArchAppUtilWindows::startService()
{
	// open service manager
	SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (mgr == NULL) {
		throw XArchDaemonFailed(new XArchEvalWindows());
	}

	// open the service
	SC_HANDLE service = OpenService(
		mgr, app().daemonName(), SERVICE_START);

	if (service == NULL) {
		CloseServiceHandle(mgr);
		throw XArchDaemonFailed(new XArchEvalWindows());
	}

	// start the service
	if (StartService(service, 0, NULL)) {
		LOG((CLOG_INFO "service '%s' started", app().daemonName()));
	}
	else {
		throw XArchDaemonFailed(new XArchEvalWindows());
	}
}

void
CArchAppUtilWindows::stopService()
{
	// open service manager
	SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (mgr == NULL) {
		throw XArchDaemonFailed(new XArchEvalWindows());
	}

	// open the service
	SC_HANDLE service = OpenService(
		mgr, app().daemonName(),
		SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (service == NULL) {
		CloseServiceHandle(mgr);
		throw XArchDaemonFailed(new XArchEvalWindows());
	}

	// ask the service to stop, asynchronously
	SERVICE_STATUS ss;
	if (!ControlService(service, SERVICE_CONTROL_STOP, &ss)) {
		DWORD dwErrCode = GetLastError(); 
		if (dwErrCode != ERROR_SERVICE_NOT_ACTIVE) {
			LOG((CLOG_ERR "cannot stop service '%s'", app().daemonName()));
			throw XArchDaemonFailed(new XArchEvalWindows());
		}
	}

	LOG((CLOG_INFO "service '%s' stopping asynchronously", app().daemonName()));
}

static
int 
mainLoopStatic() 
{
	return CArchAppUtil::instance().app().mainLoop();
}

int 
CArchAppUtilWindows::daemonNTMainLoop(int argc, const char** argv)
{
	app().initApp(argc, argv);
	debugServiceWait();

	// NB: what the hell does this do?!
	app().argsBase().m_backend = false;
	
	return CArchMiscWindows::runDaemon(mainLoopStatic);
}

void 
CArchAppUtilWindows::exitApp(int code)
{
	switch (m_exitMode) {

		case kExitModeDaemon:
			CArchMiscWindows::daemonFailed(code);
			break;

		default:
			throw XExitApp(code);
	}
}

int daemonNTMainLoopStatic(int argc, const char** argv)
{
	return CArchAppUtilWindows::instance().daemonNTMainLoop(argc, argv);
}

int 
CArchAppUtilWindows::daemonNTStartup(int, char**)
{
	CSystemLogger sysLogger(app().daemonName(), false);
	m_exitMode = kExitModeDaemon;
	return ARCH->daemonize(app().daemonName(), daemonNTMainLoopStatic);
}

static
int
daemonNTStartupStatic(int argc, char** argv)
{
	return CArchAppUtilWindows::instance().daemonNTStartup(argc, argv);
}

static
int
foregroundStartupStatic(int argc, char** argv)
{
	return CArchAppUtil::instance().app().foregroundStartup(argc, argv);
}

void
CArchAppUtilWindows::beforeAppExit()
{
	CString name;
	CArchMiscWindows::getParentProcessName(name);

	// if the user did not launch from the command prompt (i.e. it was launched
	// by double clicking, or through a debugger), allow user to read any error
	// messages (instead of the window closing automatically).
	if (name != "cmd.exe") {
		std::cout << std::endl << "Press any key to exit..." << std::endl;
		int c = _getch();
	}
}

int
CArchAppUtilWindows::run(int argc, char** argv, CreateTaskBarReceiverFunc createTaskBarReceiver)
{
	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));

	CMSWindowsScreen::init(CArchMiscWindows::instanceWin32());
	CThread::getCurrentThread().setPriority(-14);

	StartupFunc startup;
	if (CArchMiscWindows::wasLaunchedAsService()) {
		startup = &daemonNTStartupStatic;
	} else {
		startup = &foregroundStartupStatic;
		app().argsBase().m_daemon = false;
	}

	return app().runInner(argc, argv, NULL, startup, createTaskBarReceiver);
}

CArchAppUtilWindows& 
CArchAppUtilWindows::instance()
{
	return (CArchAppUtilWindows&)CArchAppUtil::instance();
}

void 
CArchAppUtilWindows::debugServiceWait()
{
	if (app().argsBase().m_debugServiceWait)
	{
		while(true)
		{
			// this code is only executed when the process is launched via the
			// windows service controller (and --debug-service-wait arg is 
			// used). to debug, set a breakpoint on this line so that 
			// execution is delayed until the debugger is attached.
			ARCH->sleep(1);
			LOG((CLOG_INFO "waiting for debugger to attach"));
		}
	}
}

void 
CArchAppUtilWindows::startNode()
{
	if (app().argsBase().m_relaunchMode) {

		LOG((CLOG_DEBUG1 "entering relaunch mode"));
		CMSWindowsRelauncher relauncher;
		relauncher.startAsync();

		// HACK: create a dummy screen, which can handle system events 
		// (such as a stop request from the service controller).
		CScreen* dummyScreen = app().createScreen();
	}
	else {
		app().startNode();
	}
}
