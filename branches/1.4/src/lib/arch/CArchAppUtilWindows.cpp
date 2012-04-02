/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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
		throw XArch(new XArchEvalWindows());
    }
}

CArchAppUtilWindows::~CArchAppUtilWindows()
{
}

BOOL WINAPI CArchAppUtilWindows::consoleHandler(DWORD CEvent)
{
	if (instance().app().m_taskBarReceiver)
	{
		// HACK: it would be nice to delete the s_taskBarReceiver object, but 
		// this is best done by the CApp destructor; however i don't feel like
		// opening up that can of worms today... i need sleep.
		instance().app().m_taskBarReceiver->cleanup();
	}

	ExitProcess(kExitTerminated);
    return TRUE;
}

bool 
CArchAppUtilWindows::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (app().isArg(i, argc, argv, NULL, "--service")) {

		LOG((CLOG_WARN "obsolete argument --service, use synergyd instead."));
		app().m_bye(kExitFailed);
	}
	else if (app().isArg(i, argc, argv, NULL, "--exit-pause")) {

		app().argsBase().m_pauseOnExit = true;
	}
#if GAME_DEVICE_SUPPORT
	else if (app().isArg(i, argc, argv, NULL, "--game-device")) {
		app().argsBase().m_gameDevice = true;
	}
#endif
	else {
		// option not supported here
		return false;
	}

	return true;
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
	// this can be handy for debugging, since the application is launched in
	// a new console window, and will normally close on exit (making it so
	// that we can't see error messages).
	if (app().argsBase().m_pauseOnExit) {
		std::cout << std::endl << "press any key to exit..." << std::endl;
		int c = _getch();
	}
}

int
CArchAppUtilWindows::run(int argc, char** argv)
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

	return app().runInner(argc, argv, NULL, startup);
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
		CMSWindowsRelauncher relauncher(true);
		relauncher.startAsync();

		// HACK: create a dummy screen, which can handle system events 
		// (such as a stop request from the service controller).
		CScreen* dummyScreen = app().createScreen();
	}
	else {
		app().startNode();
	}
}
