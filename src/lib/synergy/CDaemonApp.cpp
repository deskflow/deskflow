/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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

// TODO: split this class into windows and unix to get rid
// of all the #ifdefs!

#include "CDaemonApp.h"
#include "CEventQueue.h"
#include "LogOutputters.h"
#include "CLog.h"
#include "XArch.h"
#include "CApp.h"
#include "TMethodJob.h"
#include "TMethodEventJob.h"
#include "CIpcClientProxy.h"
#include "CIpcMessage.h"
#include "CSocketMultiplexer.h"
#include "CIpcLogOutputter.h"
#include "CLog.h"

#include <string>
#include <iostream>
#include <sstream>

#if SYSAPI_WIN32

#include "CArchMiscWindows.h"
#include "XArchWindows.h"
#include "CScreen.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsDebugOutputter.h"
#include "CMSWindowsWatchdog.h"
#include "CMSWindowsEventQueueBuffer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

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

#if SYSAPI_WIN32
int
winMainLoopStatic(int, const char**)
{
	return CArchMiscWindows::runDaemon(mainLoopStatic);
}
#endif

CDaemonApp::CDaemonApp() :
	m_events(nullptr),
	m_ipcServer(nullptr),
	m_ipcLogOutputter(nullptr)
	#if SYSAPI_WIN32
	,m_watchdog(nullptr)
	#endif
{
	s_instance = this;
}

CDaemonApp::~CDaemonApp()
{
}

int
CDaemonApp::run(int argc, char** argv)
{
#if SYSAPI_WIN32
	// win32 instance needed for threading, etc.
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif
	
	CArch arch;
	arch.init();

	CLog log;
	CEventQueue events;
	m_events = &events;

	bool uninstall = false;
	try
	{
#if SYSAPI_WIN32
		// sends debug messages to visual studio console window.
		log.insert(new CMSWindowsDebugOutputter());
#endif

		// default log level to system setting.
		string logLevel = arch.setting("LogLevel");
		if (logLevel != "")
			log.setFilter(logLevel.c_str());

		bool foreground = false;

		for (int i = 1; i < argc; ++i) {
			string arg(argv[i]);

			if (arg == "/f" || arg == "-f") {
				foreground = true;
			}
#if SYSAPI_WIN32
			else if (arg == "/install") {
				uninstall = true;
				arch.installDaemon();
				return kExitSuccess;
			}
			else if (arg == "/uninstall") {
				arch.uninstallDaemon();
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
			arch.daemonize("Synergy", winMainLoopStatic);
#elif SYSAPI_UNIX
			arch.daemonize("Synergy", unixMainLoopStatic);
#endif
		}

		return kExitSuccess;
	}
	catch (XArch& e) {
		CString message = e.what();
		if (uninstall && (message.find("The service has not been started") != CString::npos)) {
			// TODO: if we're keeping this use error code instead (what is it?!).
			// HACK: this message happens intermittently, not sure where from but
			// it's quite misleading for the user. they thing something has gone
			// horribly wrong, but it's just the service manager reporting a false
			// positive (the service has actually shut down in most cases).
		}
		else {
			foregroundError(message.c_str());
		}
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

		// create socket multiplexer.  this must happen after daemonization
		// on unix because threads evaporate across a fork().
		CSocketMultiplexer multiplexer;

		// uses event queue, must be created here.
		m_ipcServer = new CIpcServer(m_events, &multiplexer);

		// send logging to gui via ipc, log system adopts outputter.
		m_ipcLogOutputter = new CIpcLogOutputter(*m_ipcServer);
		CLOG->insert(m_ipcLogOutputter);
		
#if SYSAPI_WIN32
		m_watchdog = new CMSWindowsWatchdog(false, *m_ipcServer, *m_ipcLogOutputter);
#endif
		
		m_events->adoptHandler(
			m_events->forCIpcServer().messageReceived(), m_ipcServer,
			new TMethodEventJob<CDaemonApp>(this, &CDaemonApp::handleIpcMessage));

		m_ipcServer->listen();
		
#if SYSAPI_WIN32

		// install the platform event queue to handle service stop events.
		m_events->adoptBuffer(new CMSWindowsEventQueueBuffer(m_events));
		
		CString command = ARCH->setting("Command");
		bool elevate = ARCH->setting("Elevate") == "1";
		if (command != "") {
			LOG((CLOG_INFO "using last known command: %s", command.c_str()));
			m_watchdog->setCommand(command, elevate);
		}

		m_watchdog->startAsync();
#endif
		m_events->loop();

#if SYSAPI_WIN32
		m_watchdog->stop();
		delete m_watchdog;
#endif

		m_events->removeHandler(
			m_events->forCIpcServer().messageReceived(), m_ipcServer);
		
		CLOG->remove(m_ipcLogOutputter);
		delete m_ipcLogOutputter;
		delete m_ipcServer;
		
		DAEMON_RUNNING(false);
	}
	catch (XArch& e) {
		LOG((CLOG_ERR "xarch exception: %s", e.what().c_str()));
	}
	catch (std::exception& e) {
		LOG((CLOG_ERR "std exception: %s", e.what()));
	}
	catch (...) {
		LOG((CLOG_ERR "unrecognized error."));
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
	// TODO: move to CArchMiscWindows
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

void
CDaemonApp::handleIpcMessage(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	switch (m->type()) {
		case kIpcCommand: {
			CIpcCommandMessage* cm = static_cast<CIpcCommandMessage*>(m);
			CString command = cm->command();

			// if empty quotes, clear.
			if (command == "\"\"") {
				command.clear();
			}

			if (!command.empty()) {
				LOG((CLOG_DEBUG "new command, elevate=%d command=%s", cm->elevate(), command.c_str()));

				CString debugArg("--debug");
				UInt32 debugArgPos = static_cast<UInt32>(command.find(debugArg));
				if (debugArgPos != CString::npos) {
					UInt32 from = debugArgPos + static_cast<UInt32>(debugArg.size()) + 1;
					UInt32 nextSpace = static_cast<UInt32>(command.find(" ", from));
					CString logLevel(command.substr(from, nextSpace - from));
				
					try {
						// change log level based on that in the command string
						// and change to that log level now.
						ARCH->setting("LogLevel", logLevel);
						CLOG->setFilter(logLevel.c_str());
					}
					catch (XArch& e) {
						LOG((CLOG_ERR "failed to save LogLevel setting, %s", e.what().c_str()));
					}
				}
			}
			else {
				LOG((CLOG_DEBUG "empty command, elevate=%d", cm->elevate()));
			}

			try {
				// store command in system settings. this is used when the daemon
				// next starts.
				ARCH->setting("Command", command);

				// TODO: it would be nice to store bools/ints...
				ARCH->setting("Elevate", CString(cm->elevate() ? "1" : "0"));
			}
			catch (XArch& e) {
				LOG((CLOG_ERR "failed to save settings, %s", e.what().c_str()));
			}

#if SYSAPI_WIN32
			// tell the relauncher about the new command. this causes the
			// relauncher to stop the existing command and start the new
			// command.
			m_watchdog->setCommand(command, cm->elevate());
#endif
			break;
		}

		case kIpcHello:
			CIpcHelloMessage* hm = static_cast<CIpcHelloMessage*>(m);
			CString type;
			switch (hm->clientType()) {
				case kIpcClientGui: type = "gui"; break;
				case kIpcClientNode: type = "node"; break;
				default: type = "unknown"; break;
			}

			LOG((CLOG_DEBUG "ipc hello, type=%s", type.c_str()));

#if SYSAPI_WIN32
			CString watchdogStatus = m_watchdog->isProcessActive() ? "ok" : "error";
			LOG((CLOG_INFO "watchdog status: %s", watchdogStatus.c_str()));
#endif

			m_ipcLogOutputter->notifyBuffer();
			break;
	}
}
