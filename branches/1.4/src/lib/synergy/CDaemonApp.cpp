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
#include "CEventQueue.h"
#include "LogOutputters.h"
#include "CLog.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#include "XArchWindows.h"
#include "CScreen.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsRelauncher.h"
#include "CMSWindowsDebugOutputter.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <string>
#include <iostream>
#include <sstream>

using namespace std;

CDaemonApp* CDaemonApp::s_instance = NULL;

int
mainLoopStatic()
{
	CDaemonApp::s_instance->mainLoop(true);
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

CDaemonApp::CDaemonApp()
{
	s_instance = this;
}

CDaemonApp::~CDaemonApp()
{
}

int
CDaemonApp::run(int argc, char** argv)
{
	try
	{
#if SYSAPI_WIN32
		// win32 instance needed for threading, etc.
		CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

		// create singletons.
		CArch arch;
		CLog log;

#if SYSAPI_WIN32
		// sends debug messages to visual studio console window.
		CLOG->insert(new CMSWindowsDebugOutputter());
#endif

		// default log level to system setting.
		string logLevel = ARCH->setting("LogLevel");
		if (logLevel != "")
			CLOG->setFilter(logLevel.c_str());

		bool foreground = false;

		for (int i = 1; i < argc; ++i) {
			string arg(argv[i]);

			if (arg == "/f" || arg == "-f") {
				foreground = true;
			}
			else if (arg == "/l" || arg == "-l") {
				const char* logFilter = argv[++i];
				if (!CLOG->setFilter(logFilter)) {
					stringstream ss;
					ss << "Unrecognized log level: " << logFilter;
					foregroundError(ss.str().c_str());
					return kExitArgs;
				}
			}
#if SYSAPI_WIN32
			else if (arg == "/install") {
				ARCH->installDaemon();
				return kExitSuccess;
			}
			else if (arg == "/uninstall") {
				ARCH->uninstallDaemon();
				return kExitSuccess;
			}
#endif
			else {
				stringstream ss;
				ss << "Unrecognized argument: " << arg;
				foregroundError(ss.str().c_str());
				return kExitArgs;
			}
		}

		if (foreground) {
			// run process in foreground instead of daemonizing.
			// useful for debugging.
			mainLoop(false);
		}
		else {
#if SYSAPI_WIN32
			ARCH->daemonize("Synergy", winMainLoopStatic);
#else SYSAPI_UNIX
			ARCH->daemonize("Synergy", unixMainLoopStatic);
#endif
		}

		return kExitSuccess;
	}
	catch (XArch& e) {
		foregroundError(e.what().c_str());
		return kExitFailed;
	}
	catch (std::exception& e) {
		foregroundError(e.what());
		return kExitFailed;
	}
	catch (...) {
		foregroundError("Unrecognized error.");
		return kExitFailed;
	}
}

void
CDaemonApp::mainLoop(bool logToFile)
{
	try
	{
		DAEMON_RUNNING(true);

		if (logToFile)
			CLOG->insert(new CFileLogOutputter(logPath().c_str()));

		CEventQueue eventQueue;

#if SYSAPI_WIN32
		// HACK: create a dummy screen, which can handle system events 
		// (such as a stop request from the service controller).
		CMSWindowsScreen::init(CArchMiscWindows::instanceWin32());
		CScreen dummyScreen(new CMSWindowsScreen(false, true, false));

		CMSWindowsRelauncher relauncher(false);
		relauncher.startAsync();
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
	catch (XArch& e) {
		LOG((CLOG_ERR, e.what().c_str()));
	}
	catch (std::exception& e) {
		LOG((CLOG_ERR, e.what()));
	}
	catch (...) {
		LOG((CLOG_ERR, "Unrecognized error."));
	}
}

void
CDaemonApp::foregroundError(const char* message)
{
#if SYSAPI_WIN32
	MessageBox(NULL, message, "Synergy Service", MB_OK | MB_ICONERROR);
#elif SYSAPI_UNIX
	cerr << message << endl;
#endif
}

std::string
CDaemonApp::logPath()
{
#ifdef SYSAPI_WIN32
	// on windows, log to the same dir as the binary.
	char fileNameBuffer[MAX_PATH];
	GetModuleFileName(NULL, fileNameBuffer, MAX_PATH);
	string fileName(fileNameBuffer);
	size_t lastSlash = fileName.find_last_of("\\");
	string path(fileName.substr(0, lastSlash));
	path.append("\\").append(LOG_FILENAME);
	return path;
#elif SYSAPI_UNIX
	return "/var/log/" LOG_FILENAME;
#endif
}
